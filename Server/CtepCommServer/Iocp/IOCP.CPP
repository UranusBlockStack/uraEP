#include "stdafx.h"

#include "iocp.h"
#pragma comment(lib, "WS2_32.lib")

Log4CppLib CIOCPServer::m_log("Iocp");
CIOCPServer::CIOCPServer()
{
	m_queFreeContext.Initialize(true, true, true);
	m_queFreePacket.Initialize(true, true, true);
	m_quePendingAccepts.Initialize(true, true, true);
	m_queConnectionList.Initialize(true, true, true);

	FindMoudleInterface(CLoadModules::CtepTransServer);
	FindMoudleInterface(CLoadModules::CtepAppServer);

	// Accept请求
	m_hAcceptEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hRepostEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL); 
	m_nRepostCount = 0;

	m_nMaxAccepts = 150;
	m_nMaxSends = 100;
	m_nMaxFreeBuffers = 2000;
	m_nMaxConnections = 3000;

	m_nIocpOutStandingSend = 0;
	m_nIocpOutStandingRecv = 0;
	m_nIocpOutStandingListen = 0;

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	m_dwWorkThreadCount = si.dwNumberOfProcessors*2;
	if ( m_dwWorkThreadCount < 4)
	{
		m_dwWorkThreadCount = 4;
	}
	else if ( m_dwWorkThreadCount > MAX_THREAD)
	{
		m_dwWorkThreadCount = MAX_THREAD;
	}

	m_hListenThread = NULL;
	m_hCompletion = NULL;

	m_bShutDown = FALSE;
	m_bServerStarted = FALSE;
}

CIOCPServer::~CIOCPServer()
{
	Shutdown();

	if(m_hListenThread != NULL)
		::CloseHandle(m_hListenThread);

	::CloseHandle(m_hRepostEvent);
	::CloseHandle(m_hAcceptEvent);

}


ReadWritePacket *CIOCPServer::AllocateBuffer(StTransferChannel* pContext, EmPacketOperationType type, int nLen/* = 0*/)
{
	ReadWritePacket *pBuffer = NULL;
	char* extraBuf = NULL;

	if( nLen > CTEP_DEFAULT_BUFFER_SIZE)
	{
		extraBuf = (char*)::HeapAlloc(GetProcessHeap(), 
			HEAP_ZERO_MEMORY, nLen + CTEP_PACKET_HEADER_SIZE);
		if ( !extraBuf)
			return NULL;
	}

	// 为缓冲区对象申请内存
	pBuffer = m_queFreePacket.Pop();
	if( !pBuffer)  // 内存池为空，申请新的内存
	{
		pBuffer = (ReadWritePacket *)::HeapAlloc(GetProcessHeap(), 
			HEAP_ZERO_MEMORY, sizeof(ReadWritePacket));
	}

	// 初始化新的缓冲区对象
	if( pBuffer)
	{
		pBuffer->PacketInit();
		pBuffer->opType = type;
		pBuffer->pTransChn = pContext;
		if (pContext)
			pBuffer->piTrans = pContext->piTrans;

		if ( extraBuf)
		{
			pBuffer->buff.maxlength = nLen;
			pBuffer->buff.buff = extraBuf;
		}
		pBuffer->buff.size = nLen;
	}

	return pBuffer;
}

void CIOCPServer::ReleaseBuffer(ReadWritePacket *pBuffer)
{
	if ( pBuffer->buff.buff != pBuffer->cBuf)
	{
		ASSERT(pBuffer->buff.buff);
		::HeapFree(::GetProcessHeap(), 0, pBuffer->buff.buff);
	}

	if( m_queFreePacket.GetCount() <= m_nMaxFreeBuffers)	// 将要释放的内存添加到空闲列表中
	{
		pBuffer->PacketInit();
#if _DEBUG
		pBuffer->buff.buff = (char*)11;
#endif
	}
	else	// 已经达到最大值，真正的释放内存
	{
		::HeapFree(::GetProcessHeap(), 0, pBuffer);
	}
}


StTransferChannelEx *CIOCPServer::AllocateContext(HANDLE s, ICTEPTransferProtocolServer* piTrans)
{
	StTransferChannelEx *pContext;

	ASSERT(piTrans && s != INVALID_HANDLE_VALUE && s != 0);

	// 申请一个StTransferChannelEx对象
	ASSERT(m_queFreeContext.GetCount()>200);
	pContext = m_queFreeContext.Pop();
	ASSERT(pContext);
	// 初始化对象成员
	if( pContext)
	{
		pContext->InitEx();
		pContext->head.hFile = s;
		pContext->head.piTrans = piTrans;
		pContext->bClosing = FALSE;
		if ( piTrans == m_TS[0])
		{
			pContext->head.type = EnTransferChannelType::TCP;
		}
		else if ( piTrans == m_TS[1])
		{
			pContext->head.type = EnTransferChannelType::UDP;
		}
		else
		{
			pContext->head.type = EnTransferChannelType::OTHER;
		}
	}
	return pContext;
}

void CIOCPServer::FreeBuffers()
{
	// 遍历m_pFreeBufferList空闲列表，释放缓冲区池内存
	m_queFreePacket.Lock();

	ReadWritePacket *pFreeBuffer = m_queFreePacket.Pop();
	while(pFreeBuffer != NULL)
	{
		if(!::HeapFree(::GetProcessHeap(), 0, pFreeBuffer))
		{
#ifdef _DEBUG
			::OutputDebugString(L"  FreeBuffers释放内存出错！");
			ASSERT(0);
#endif // _DEBUG
			break;
		}
		pFreeBuffer = m_queFreePacket.Pop();
	}

	m_queFreePacket.Unlock();
}

void CIOCPServer::FreeContexts()
{
	// 遍历m_pFreeBufferList空闲列表，释放缓冲区池内存
	m_queFreeContext.Lock();

	StTransferChannelEx *pFreeContext = m_queFreeContext.Pop();
	while(pFreeContext != NULL)
	{
		if(!::HeapFree(::GetProcessHeap(), 0, pFreeContext))
		{
#ifdef _DEBUG
			::OutputDebugString(L"  FreeBuffers释放内存出错！");
			ASSERT(0);
#endif // _DEBUG
			break;
		}
		pFreeContext = m_queFreeContext.Pop();
	}
	m_queFreeContext.Unlock();
}


BOOL CIOCPServer::AddAConnection(StTransferChannelEx *pContext)
{
	// 向客户连接列表添加一个StTransferChannelEx对象

	if(m_queConnectionList.GetCount() <= m_nMaxConnections)
	{
		// 添加到表头
		m_queConnectionList.Push(pContext);
		return TRUE;
	}

	return FALSE;
}

void CIOCPServer::CloseAConnection(StTransferChannelEx *pContext)
{
	if ( pContext->bClosing == TRUE)
		return ;

	::EnterCriticalSection(&pContext->head.Lock);
	if ( pContext->bClosing != TRUE)
	{
		pContext->bClosing = TRUE;

		// 首先从列表中移除要关闭的连接
		m_queConnectionList.PopFind(pContext);

		// 然后关闭客户套节字
		pContext->head.piTrans->Disconnect(&pContext->head);

		ASSERT(pContext->pNext == 0);
		m_queFreeContext.Push(pContext);
	}
	::LeaveCriticalSection(&pContext->head.Lock);
}

void CIOCPServer::CloseAllConnections()
{
	// 遍历整个连接列表，关闭所有的客户套节字
	StTransferChannelEx *pContext = m_queConnectionList.Pop();
	while(pContext != NULL)
	{	
		::EnterCriticalSection(&pContext->head.Lock);
		pContext->bClosing = TRUE;

		// 然后关闭客户套节字
		pContext->head.piTrans->Disconnect(&pContext->head);

		ASSERT(pContext->pNext == 0);
		m_queFreeContext.Push(pContext);
		::LeaveCriticalSection(&pContext->head.Lock);

		pContext = m_queConnectionList.Pop();
	}
}


BOOL CIOCPServer::InsertPendingAccept(ReadWritePacket *pBuffer)
{
	// 将一个I/O缓冲区对象插入到m_pPendingAccepts表中
	InterlockedIncrement(&m_nIocpOutStandingListen);
	m_quePendingAccepts.Push(pBuffer);
	return TRUE;
}

BOOL CIOCPServer::RemovePendingAccept(ReadWritePacket *pBuffer)
{
	// 遍历m_pPendingAccepts表，从中移除pBuffer所指向的缓冲区对象
	InterlockedDecrement(&m_nIocpOutStandingListen);
	return nullptr != m_quePendingAccepts.PopFind(pBuffer);
}


BOOL CIOCPServer::Start(int nMaxConnections, int nMaxFreeBuffers)
{
	BOOL bResult = FALSE;

	// 检查服务是否已经启动
	if(m_bServerStarted)
		return FALSE;

	// 保存用户参数
	m_nMaxConnections = nMaxConnections;
	m_nMaxFreeBuffers = nMaxFreeBuffers;

	// 初始化状态变量
	m_bShutDown = FALSE;
	m_bServerStarted = TRUE;

	// 创建完成端口对象
	m_hCompletion = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	if ( m_hCompletion == NULL)
		return FALSE;

	HRESULT hr = E_FAIL;
	for ( DWORD i=0; i< m_TransCount; i++)
	{
		
		if ( m_TS[i])
		{
			if ( SUCCEEDED(m_TS[i]->InitializeCompletePort(dynamic_cast<ICTEPTransferProtocolCallBack*>(this))))
			{
				hr = S_OK;
			}
			else
			{
				m_TS[i]->Final();
				m_TS[i] = NULL;
			}
		}
	}
	if ( FAILED(hr))
	{
		m_log.FmtError(5, L"Server initialize failed. Error code : 0x%08x", hr);
		goto End;
	}

	if ( !OnStart())
	{
		m_log.FmtError(5, L"OnStart initialize failed. Error code : 0x%08x", hr);
#ifndef _DEBUG
		goto End;
#endif // _DEBUG
	}


	// 创建内存池
	long nDefaultFreePacket = max(m_nMaxFreeBuffers/10, m_nMaxConnections/5);
	for (long i = m_queFreePacket.GetCount(); i < nDefaultFreePacket; i++)
	{
		ReadWritePacket* pBuffer = (ReadWritePacket *)::HeapAlloc(GetProcessHeap(), 
			HEAP_ZERO_MEMORY, sizeof(ReadWritePacket));
		if ( pBuffer)
		{
			pBuffer->PacketInit();
			m_queFreePacket.Push(pBuffer);
		}
	}

	for (long i = m_queFreeContext.GetCount(); i < m_nMaxConnections + 1000; i++)
	{
		StTransferChannelEx* pContext = (StTransferChannelEx *)
			::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(StTransferChannelEx));
		if ( pContext)
		{
			pContext->InitEx();
			::InitializeCriticalSection(&pContext->head.Lock);
			m_queFreeContext.Push(pContext);
		}
	}

	// 创建监听线程
	return _ListenThreadProc(this);
	//m_hListenThread = ::CreateThread(NULL, 0, _ListenThreadProc, this, 0, NULL);
	//bResult = TRUE;

End:
	if ( FALSE == bResult)
	{
		m_bServerStarted = FALSE;
	}

	return bResult;
}

void CIOCPServer::Shutdown()
{
	if( !m_bServerStarted)
		return;

	OnShutdown();

	// 通知监听线程，马上停止服务
	m_bShutDown = TRUE;
	::SetEvent(m_hAcceptEvent);

	// 等待监听线程退出
	::WaitForSingleObject(m_hListenThread, INFINITE);
	::CloseHandle(m_hListenThread);
	m_hListenThread = NULL;

	m_bServerStarted = FALSE;
}

DWORD CIOCPServer::_threadListen()
{
	// 先在监听套节字上投递几个Accept I/O
	HRESULT hr = E_FAIL;
	for (DWORD i=0; i< m_TransCount; i++)
	{
		if ( m_TS[i])
		{
			if ( SUCCEEDED(m_TS[i]->PostListen(true)))
			{
				hr = S_OK;
			}
			else
			{
				m_TS[i]->Final();
				m_TS[i] = nullptr;
			}
		}
	}

	if ( FAILED(hr))
	{
		return (DWORD)-1;
	}

	// 构建事件对象数组，以便在上面调用WSAWaitForMultipleEvents函数
	HANDLE hWaitEvents[2 + MAX_THREAD];
	int nEventCount = 0;
	hWaitEvents[nEventCount ++] = m_hAcceptEvent;
	hWaitEvents[nEventCount ++] = m_hRepostEvent;

	// 创建指定数量的工作线程在完成端口上处理I/O
	for(ULONG i=0; i<m_dwWorkThreadCount; i++)
	{
		hWaitEvents[nEventCount ++] = ::CreateThread(NULL, 0, _WorkerThreadProc, this, 0, NULL);
	}

	// 下面进入无限循环，处理事件对象数组中的事件
	while(TRUE)
	{
		int nIndex = ::WSAWaitForMultipleEvents(nEventCount, hWaitEvents, FALSE, 60*1000, FALSE);

		// 首先检查是否要停止服务
		if( m_bShutDown || nIndex == WSA_WAIT_FAILED)
		{
			// 关闭所有连接
			CloseAllConnections();
			::Sleep(50);		// 给I/O工作线程一个执行的机会

			// 关闭监听套节字
			m_quePendingAccepts.Lock();
			ReadWritePacket* pPacket = m_quePendingAccepts.GetHead();
			while( pPacket != NULL)
			{
				pPacket->piTrans->Disconnect(0, pPacket);
				pPacket = pPacket->pNext;
			}
			m_quePendingAccepts.Unlock();
			::Sleep(50);

			for (DWORD i=0;i<m_TransCount;i++)
			{
				if ( m_TS[i])
				{
					m_TS[i]->Final();
				}
			}
			::Sleep(500);		// 给I/O工作线程一个执行的机会

			ASSERT(m_nIocpOutStandingListen == 0
				&& m_nIocpOutStandingRecv == 0
				&& m_nIocpOutStandingSend == 0);

			// 通知所有I/O处理线程退出
			for(ULONG i=0; i<m_dwWorkThreadCount; i++)
			{	
				::PostQueuedCompletionStatus(m_hCompletion, (DWORD)-1, 0, NULL);
			}

			// 等待I/O处理线程退出
			::WaitForMultipleObjects(m_dwWorkThreadCount, &hWaitEvents[2], TRUE, 5*1000);

			for(ULONG i=2; i<m_dwWorkThreadCount + 2; i++)
			{	
				::CloseHandle(hWaitEvents[i]);
			}

			::CloseHandle(m_hCompletion);

			FreeBuffers();
			FreeContexts();
			return 0;
		}	

		// 1）定时检查所有未返回的AcceptEx I/O的连接建立了多长时间
		if(nIndex == WSA_WAIT_TIMEOUT)
		{
			m_quePendingAccepts.Lock();
			ReadWritePacket* pPacket = m_quePendingAccepts.GetHead();
			while( pPacket != NULL)
			{
				long nSeconds = pPacket->piTrans->GetDuration(pPacket);

				// 如果超过2分钟客户还不发送初始数据，就让这个客户go away
				if(nSeconds != -1 && (nSeconds > 2*60 || nSeconds < 0))
				{
					pPacket->piTrans->Disconnect(0, pPacket);
				}

				pPacket = pPacket->pNext;
			}
			m_quePendingAccepts.Unlock();
		}
		else
		{
			nIndex = nIndex - WAIT_OBJECT_0;
			int nLimit = 0;
			if(nIndex == 0)			// 2）m_hAcceptEvent事件对象受信，说明投递的Accept请求不够，需要增加
			{
				if ( m_TS[0])
				{
					WSANETWORKEVENTS ne;
					::WSAEnumNetworkEvents(m_TS[0]->GetListenSocket(), hWaitEvents[nIndex], &ne);
					if(ne.lNetworkEvents & FD_ACCEPT)
					{
						nLimit = 50;  // 增加的个数，这里设为50个
					}
				}
				else
				{
					ASSERT(0);
					ResetEvent(m_hAcceptEvent);
					nLimit = 50;
				}
			}
			else if(nIndex == 1)	// 3）m_hRepostEvent事件对象受信，说明处理I/O的线程接受到新的客户
			{
				nLimit = InterlockedExchange(&m_nRepostCount, 0);
			}
			else if(nIndex > 1)		// I/O服务线程退出，说明有错误发生，关闭服务器
			{
				m_bShutDown = TRUE;
				continue;
			}

			// 投递nLimit个AcceptEx I/O请求
			int i = 0;
			while( i++ < nLimit && m_quePendingAccepts.GetCount() < m_nMaxAccepts)
			{
				if ( m_TS[0])
				{
					m_TS[0]->PostListen();
				}
			}
		}
	}
	return 0;
}

DWORD CIOCPServer::_threadWorker()
{
#ifdef _DEBUG
	m_log.wprint(L"	WorkerThread 启动... \n");
#endif // _DEBUG
	ReadWritePacket *pBuffer;
	ULONG_PTR dwKey;
	DWORD dwTrans;
	LPOVERLAPPED lpol;
	while(TRUE)
	{
		// 在关联到此完成端口的所有套节字上等待I/O完成
		BOOL bOK = ::GetQueuedCompletionStatus(m_hCompletion, 
			&dwTrans, &dwKey, (LPOVERLAPPED*)&lpol, WSA_INFINITE);

		if(dwTrans == -1 && lpol == 0 && dwKey == 0) // 用户通知退出
		{
#ifdef _DEBUG
			m_log.wprint(L"	WorkerThread 退出 \n");
#endif // _DEBUG

			::PostQueuedCompletionStatus(m_hCompletion, (DWORD)-1, 0, NULL);
			return 0;
		}

		StTransferChannelEx* pContext = (StTransferChannelEx*)dwKey;
		pBuffer = CONTAINING_RECORD(lpol, ReadWritePacket, ol);
		int nError = NO_ERROR;
		if( !bOK)						// 在此套节字上有错误发生
		{
			dwTrans = pBuffer->ol.InternalHigh;
			nError = pBuffer->ol.Internal;
			m_log.FmtWarning(3, L"在此套节字上有错误发生(%x)! pContext:0x%08x dwTrans:%d ErrCode:%d"
				,pBuffer->hFile, pContext, dwTrans, nError);
			ASSERT(nError != ERROR_IO_PENDING && dwTrans == 0);
		}

		if ( pBuffer->opType == EmPacketOperationType::OP_Listen)
		{
			ASSERT(pContext == NULL);
			if ( pBuffer->piTrans == m_TS[0])	// 如果是TCP
			{
				ASSERT(nError != 0 || dwTrans != 0);
			}
		}

		HandleIO(pContext, pBuffer, dwTrans, nError);
	}

#ifdef _DEBUG
	m_log.wprint(L"	WorkerThread 退出 \n");
#endif // _DEBUG
	return 0;
}


void CIOCPServer::HandleIO(StTransferChannelEx *pContext, ReadWritePacket *pBuffer, DWORD dwTrans, int nError)
{
	EmPacketOperationType ePacketType = pBuffer->opType;
	if( pContext && pContext->bClosing)
	{
		m_log.FmtMessage(1, L"	检查到套节字已经被我们关闭 \n");

		ReleaseBuffer(pBuffer);	

		// 套节字上的未决I/O计数
		if( ePacketType == EmPacketOperationType::OP_Recv)
		{
			InterlockedDecrement(&m_nIocpOutStandingRecv);
		}
		else if( ePacketType == EmPacketOperationType::OP_Send)
		{
			InterlockedDecrement(&m_nIocpOutStandingSend);
			InterlockedDecrement(&pContext->nOutstandingSend);
		}

		return;
	}
	else if ( !pContext)
	{
		RemovePendingAccept(pBuffer);// Listen操作, 出等待列表
	}

	// 检查套节字上发生的错误，如果有的话，通知用户，然后关闭套节字
	if( nError != NO_ERROR)
	{
		if(pBuffer->opType != EmPacketOperationType::OP_Listen)
		{
			if ( InterlockedExchange((volatile UINT*)&pContext->bNeedNotificationClosing, FALSE))
			{
				if ( !OnConnectionError(pContext, pBuffer, nError))
					OnConnectionClosing(pContext, pBuffer);
			}

			CloseAConnection(pContext);
			m_log.FmtMessage(1, L"	检查到客户套节字上发生错误 \n");
		}
		else
		{ // 在监听套节字上发生错误，也就是监听套节字处理的客户出错了
			if ( pBuffer->piTrans)
			{
				pBuffer->piTrans->Disconnect(0, pBuffer);
			}
			m_log.FmtMessage(1, L"	检查到监听套节字上发生错误 \n");
		}

		ReleaseBuffer(pBuffer);
		return;
	}

	// 开始处理
#ifdef _DEBUG
	ASSERT(dwTrans == 0 || pBuffer->buff.size == 0 || dwTrans == pBuffer->buff.size);
#endif // _DEBUG
	
	pBuffer->buff.size = dwTrans;
	if ( pBuffer->opType == EmPacketOperationType::OP_Listen)
	{
		if( pBuffer->hFile == INVALID_HANDLE_VALUE)
		{
			m_log.FmtMessage(1, L"	监听套节字上客户端关闭 \n");
		}
		else
		{
			// 为新接受的连接申请客户上下文对象
			StTransferChannelEx *pClient = AllocateContext(pBuffer->hFile, pBuffer->piTrans);
			if(pClient)
			{
				if( AddAConnection(pClient))
				{	
					pClient->head.piTrans->CompleteListen(&pClient->head, pBuffer);

					// 关联新连接到完成端口对象
					::CreateIoCompletionPort(pClient->head.hFile, m_hCompletion, (DWORD)pClient, 0);

					// 通知用户
					pClient->bNeedNotificationClosing = TRUE;
					OnConnectionEstablished(pClient, pBuffer);

					// 向新连接投递几个Read请求，这些空间在套节字关闭或出错时释放
					if ( SUCCEEDED(pClient->head.piTrans->PostRecv(&pClient->head, true)))
					{
						InterlockedIncrement(&m_nIocpOutStandingRecv);
					}
					else
					{
						if ( InterlockedExchange((volatile UINT*)&pClient->bNeedNotificationClosing, FALSE))
						{
							OnConnectionClosing(pClient, 0);
						}
						CloseAConnection(pClient);
					}
				}
				else	// 连接数量已满，关闭连接
				{
					CloseAConnection(pClient);
				}
			}
			else
			{
				// 资源不足，关闭与客户的连接即可
				pBuffer->piTrans->Disconnect(0, pBuffer);
			}
		}

		// Accept请求完成，释放I/O缓冲区
		ReleaseBuffer(pBuffer);	

		// 通知监听线程继续再投递一个Accept请求
		::InterlockedIncrement(&m_nRepostCount);
		::SetEvent(m_hRepostEvent);
	}
	else if(pBuffer->opType == EmPacketOperationType::OP_Recv)
	{
		if(dwTrans == 0)	// 对方关闭套节字
		{
			// 先通知用户
			if ( InterlockedExchange((volatile UINT*)&pContext->bNeedNotificationClosing, FALSE))
			{
				OnConnectionClosing(pContext, pBuffer);
			}

			// 再关闭连接
			CloseAConnection(pContext);
			ReleaseBuffer(pBuffer);	
		}
		else
		{
			OnReadCompleted(pContext, pBuffer);// 通知用户
			ReleaseBuffer(pBuffer);
		}
	}
	else if(pBuffer->opType == EmPacketOperationType::OP_Send)
	{

		if(dwTrans == 0)	// 对方关闭套节字
		{
			// 先通知用户
			if ( InterlockedExchange((volatile UINT*)&pContext->bNeedNotificationClosing, FALSE))
			{
				OnConnectionClosing(pContext, pBuffer);
			}

			// 再关闭连接
			CloseAConnection(pContext);
			ReleaseBuffer(pBuffer);	
		}
		else
		{
			// 写操作完成，通知用户
			OnWriteCompleted(pContext, pBuffer);
			// 释放SendText函数申请的缓冲区
			ReleaseBuffer(pBuffer);
		}
	}

	if ( pContext)
	{
		if( ePacketType == EmPacketOperationType::OP_Recv)
		{
			InterlockedDecrement(&m_nIocpOutStandingRecv);
		}
		else if( ePacketType == EmPacketOperationType::OP_Send)
		{
			InterlockedDecrement(&m_nIocpOutStandingSend);
			InterlockedDecrement(&pContext->nOutstandingSend);
		}


		if ( !pContext->bClosing && ePacketType == EmPacketOperationType::OP_Recv)
		{
			// 继续投递一个新的接收请求
			if ( SUCCEEDED(pContext->head.piTrans->PostRecv(&pContext->head)))
			{
				InterlockedIncrement(&m_nIocpOutStandingRecv);
			}
			else
			{
				CloseAConnection(pContext);
			}
		}
	}
}


BOOL CIOCPServer::SendText(StTransferChannelEx *pContext, char *pszText, int nLen, EmPacketLevel level)
{
	// 跟踪投递的发送的数量，防止用户仅发送数据而不接收，导致服务器抛出大量发送操作
	if(pContext->nOutstandingSend > m_nMaxSends)
	{
		m_log.FmtWarningW(3, L"PostSend overflow! %d > max(%d)"
			, pContext->nOutstandingSend, m_nMaxSends);
		return FALSE;
	}

	ReadWritePacket *pBuffer = AllocateBuffer((StTransferChannel*)pContext, OP_Send, nLen);
	if ( !pBuffer)
		return FALSE;

	memcpy(pBuffer->buff.buff, pszText, nLen);

	if ( m_mqueSendPriority.Push(pBuffer, level))
		return TRUE;

	return sendMQUEPacket(pBuffer);
}

BOOL CIOCPServer::SendPacket(StTransferChannelEx *pContext, ReadWritePacket* pBuffer, EmPacketLevel level)
{
	// 跟踪投递的发送的数量，防止用户仅发送数据而不接收，导致服务器抛出大量发送操作
	if(pContext->nOutstandingSend > m_nMaxSends)
	{
		m_log.FmtWarningW(3, L"PostSend overflow! %d > max(%d)"
			, pContext->nOutstandingSend, m_nMaxSends);
		ReleaseBuffer(pBuffer);
		return FALSE;
	}

	if ( m_mqueSendPriority.Push(pBuffer, level))
		return TRUE;

	return sendMQUEPacket(pBuffer);
}


BOOL CIOCPServer::sendMQUEPacket(ReadWritePacket* pSend)
{
	HRESULT hr;
	StTransferChannelEx *pCtxSend = (StTransferChannelEx*)pSend->pTransChn;
	ASSERT(pCtxSend && pSend->opType == EmPacketOperationType::OP_Send);
	if ( !pCtxSend || pCtxSend->bClosing)
	{
		ReleaseBuffer(pSend);
		return FALSE;
	}

	// 套节字上的未决I/O计数
	DWORD dwIocpOsSend = InterlockedIncrement(&m_nIocpOutStandingSend);
	DWORD dwOutstandingSend = InterlockedIncrement(&pCtxSend->nOutstandingSend);
	hr = pCtxSend->head.piTrans->PostSend(&pCtxSend->head, pSend);
	if ( SUCCEEDED(hr))
	{
#ifdef _DEBUG
		if ( dwIocpOsSend > MQUE_MAX_SEND+5)
		{
			m_log.FmtMessageW(5, L"PostSend OutstandingSend Count:%d-%d, QueueCount:%d"
				, dwOutstandingSend, dwIocpOsSend, m_mqueSendPriority.GetCount());
		}
#endif // _DEBUG

		return TRUE;
	}

	// 套节字上的未决I/O计数
	InterlockedDecrement(&m_nIocpOutStandingSend);
	InterlockedDecrement(&pCtxSend->nOutstandingSend);
	pCtxSend->head.piTrans->Disconnect(&pCtxSend->head);

	m_log.FmtWarning(5, L"sendMQUEPacket-PostSend Failed. SendSize:(%d) hr:0x%08x(%d)"
		, pSend->buff.size, hr, hr);
	ReleaseBuffer(pSend);

	return FALSE;
}


void CIOCPServer::OnWriteCompleted(StTransferChannelEx * /*pContext*/, ReadWritePacket * /*pBuffer*/)
{
	if ( m_mqueSendPriority.IsUsed())
	{
		if ( m_nIocpOutStandingSend < MQUE_MIN_SEND)
		{
			if ( m_mqueSPLock.TryLock())
			{
				m_log.FmtMessage(5, L"WriteCompleted-MQue-Pop. IocpOsSend:%d QueueCount:%d"
					, m_nIocpOutStandingSend, m_mqueSendPriority.GetCount());
				ReadWritePacket* pSend = m_mqueSendPriority.Pop();
				while ( pSend)
				{
					sendMQUEPacket(pSend);
					if ( m_nIocpOutStandingSend > MQUE_MAX_SEND)
						break;

					pSend = m_mqueSendPriority.Pop();
				}

				if ( !pSend)// 队列为空
				{
					m_mqueSendPriority.Lock();
					pSend = m_mqueSendPriority.Pop();
					while(pSend)
					{
						sendMQUEPacket(pSend);
						pSend = m_mqueSendPriority.Pop();
					}

					ASSERT(m_mqueSendPriority.GetCount() == 0);
#ifdef _DEBUG
					m_log.FmtMessageW(5, L"OnWriteCompleted CloseMQUE. IocpOsSend:%d"
						, m_nIocpOutStandingSend);
#endif
					m_mqueSendPriority.SetUsed(FALSE);
					m_mqueSendPriority.Unlock();
				}

				m_mqueSPLock.ReleaseLock();
			}
		}
	}
	else if ( m_nIocpOutStandingSend >= MQUE_MAX_SEND)
	{
		m_mqueSendPriority.SetUsed(TRUE);
#ifdef _DEBUG
		m_log.FmtMessageW(5, L"OnWriteCompleted OpenMQUE. IocpOsSend:%d", m_nIocpOutStandingSend);
#endif
	}
}

