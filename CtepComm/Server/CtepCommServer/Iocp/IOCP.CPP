#include "stdafx.h"

#include "Interface/CTEP_Common_Struct_Ex.h"
#include "iocp.h"

#pragma comment(lib, "WS2_32.lib")

#define CTEP_DEBUG_SENDPACKET_LEVEL_OFF		0

Log4CppLib CIOCPServer::m_log("Iocp");

CIOCPServer::CIOCPServer()
{
	{
		ReadWritePacket* pBuffer = (ReadWritePacket *)::HeapAlloc(GetProcessHeap(), 
			HEAP_ZERO_MEMORY, sizeof(ReadWritePacket));
		ASSERT(pBuffer);
		pBuffer->PacketInit();
		m_queFreePacket.Initialize(pBuffer, true, true, 1);
	}
	{
		CTransferChannelEx* pContext = new CTransferChannelEx();
		ASSERT(pContext);
		m_queFreeContext.Initialize(pContext, true, true, 150);
	}
	m_quePendingAccepts.Initialize(true, true, true);
	//m_queConnectionList.Initialize(true, true, true);

	// 创建内存池
	for (long i = m_queFreePacket.GetCount(); i < 500; i++)
	{
		ReadWritePacket* pBuffer = (ReadWritePacket *)::HeapAlloc(GetProcessHeap(), 
			HEAP_ZERO_MEMORY, sizeof(ReadWritePacket));
		if ( pBuffer)
		{
			pBuffer->PacketInit();
			m_queFreePacket.Push(pBuffer);
		}
	}

	for (long i = m_queFreeContext.GetCount(); i < 300; i++)
	{
		CTransferChannelEx* pContext = new CTransferChannelEx();
		if ( pContext)
		{
			m_queFreeContext.Push(pContext);
		}
	}

	FindMoudleInterface(CLoadModules::CtepTransServer);
	FindMoudleInterface(CLoadModules::CtepAppServer);

	// Accept请求
	m_hAcceptEvent = ::WSACreateEvent();
	ASSERT(WSA_INVALID_EVENT != m_hAcceptEvent);
	m_hRepostEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	ASSERT(m_hRepostEvent);
	m_nRepostCount = 0;

	m_nMaxAccepts = 150;
	m_nMaxSends = 1000;
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

#ifdef _DEBUG
	//m_dwWorkThreadCount = 1;
#endif // _DEBUG

	m_hListenThread = NULL;
	m_hSyncWorkThread = NULL;
	m_hCompletion = NULL;

	m_bShutDown = FALSE;
	m_bServerStarted = FALSE;
}

CIOCPServer::~CIOCPServer()
{
	Shutdown();

 	if(m_hListenThread != NULL)
 		::CloseHandle(m_hListenThread);

	if ( m_hSyncWorkThread != nullptr)
		::CloseHandle(m_hSyncWorkThread);

	::CloseHandle(m_hRepostEvent);
	::CloseHandle(m_hAcceptEvent);

}


ReadWritePacket *CIOCPServer::AllocateBuffer(CTransferChannel* pContext, EmPacketOperationType type, int nLen/* = 0*/)
{
	ReadWritePacket *pBuffer = NULL;

	ASSERT(nLen <= CTEP_DEFAULT_BUFFER_SIZE);

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
		pBuffer->buff.size = nLen;

		if (pContext)
		{
			pBuffer->piTrans = pContext->piTrans;
			pBuffer->hFile = pContext->hFile;
			pBuffer->pTransChn = pContext;
		}

		ASSERT( (type != OP_IocpRecv && type != OP_IocpSend) || pContext);
		if ( type == OP_IocpRecv)
		{
			DWORD dwSize = 0;
			pBuffer->buff.buff = ((CTransferChannelEx*)pContext)->rbufPacket.GetBuffer(&dwSize);
			ASSERT(dwSize >= CTEP_DEFAULT_BUFFER_SIZE);
			pBuffer->buff.maxlength = dwSize;
		}
	}

	return pBuffer;
}

void CIOCPServer::ReleaseBuffer(ReadWritePacket *pBuffer)
{
	if( m_queFreePacket.GetCount() <= m_nMaxFreeBuffers)	// 将要释放的内存添加到空闲列表中
	{
#if _DEBUG
		pBuffer->buff.buff = (char*)11;
#endif
		m_queFreePacket.Push(pBuffer);
	}
	else	// 已经达到最大值，真正的释放内存
	{
		::HeapFree(::GetProcessHeap(), 0, pBuffer);
	}
}
void CIOCPServer::ReleaseContext(CTransferChannelEx *pContext)
{
	ASSERT(pContext->bClosing);
	m_queFreeContext.Push(pContext);

	while ( m_queFreeContext.GetCount() > m_nMaxConnections / 5 + 150)	// 将要释放的内存添加到空闲列表中
	{
		// 已经达到最大值，真正的释放内存
		CTransferChannelEx *pDelete = m_queFreeContext.Pop();
		delete pDelete;
	}
}

#ifdef _DEBUG
DWORD DebugGetBagSize(char* pbuf, DWORD dwSize, void* pParam)
{
	UNREFERENCED_PARAMETER(pParam);
	if ( dwSize >= 4)
	{
		CTEPPacket_Header* pPacket = (CTEPPacket_Header*)pbuf;
		ASSERT(pPacket->magic == 'E');
		ASSERT(pPacket->PacketLength <= CTEP_DEFAULT_BUFFER_SIZE);
		ASSERT((pPacket->Type&CTEP_PACKET_CONTENT_MASK) <= 0x70);
		ASSERT((pPacket->Type&CTEP_PACKET_SEGMENT_MASK) <= 0x03);
	}
	if ( dwSize < 2)
	{
		return 0;
	}
	//ASSERT(dwSize >= 2);
	return *((USHORT*)pbuf);
}
#endif // _DEBUG

CTransferChannelEx *CIOCPServer::AllocateContext(HANDLE s, ICTEPTransferProtocolServer* piTrans)
{
	CTransferChannelEx *pContext;

	ASSERT(piTrans && s != INVALID_HANDLE_VALUE && s != 0);

	// 申请一个StTransferChannelEx对象
	pContext = m_queFreeContext.Pop();
	if ( !pContext)
	{
		pContext = new CTransferChannelEx();
	}

	ASSERT(pContext);
	if( pContext)
	{// 初始化对象成员
		pContext->InitEx();
#ifdef _DEBUG
		pContext->rbufPacket.fn_GetBagSize = DebugGetBagSize;
#endif // _DEBUG
		pContext->hFile = s;
		pContext->piTrans = piTrans;
		pContext->bClosing = FALSE;
		if ( piTrans == m_TS[0])
		{
			pContext->type = EnTransferChannelType::TCP;
		}
		else if ( piTrans == m_TS[1])
		{
			pContext->type = EnTransferChannelType::UDP;
		}
		else
		{
			if ( piTrans->SupportIOCP() == CTEP_TS_SUPPORT_IOCP)
			{
				pContext->type = EnTransferChannelType::IocpMain;
			}
			else if ( piTrans->SupportIOCP() == CTEP_TS_SUPPORT_SYNC)
			{
				pContext->type = EnTransferChannelType::SyncMain;
			}
			else
			{
				ASSERT(0);
				m_queFreeContext.Push(pContext);
				pContext = nullptr;
			}
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
			ASSERT(0);
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

	CTransferChannelEx *pFreeContext = m_queFreeContext.Pop();
	while(pFreeContext != NULL)
	{
		delete pFreeContext;
		pFreeContext = m_queFreeContext.Pop();
	}
	m_queFreeContext.Unlock();
}


BOOL CIOCPServer::AddAConnection(CTransferChannelEx *pContext)
{
	// 向客户连接列表添加一个StTransferChannelEx对象
	if(m_lstConnection.GetCount() <= (DWORD)m_nMaxConnections)
	{
		// 添加到表头
		m_lstConnection.AddHead(pContext);
		return TRUE;
	}

	return FALSE;
}

void CIOCPServer::CloseAConnection(CTransferChannelEx *pContext)
{
	if ( pContext->bClosing == TRUE)
		return ;

	::EnterCriticalSection(&pContext->lckSend);
	if ( pContext->bClosing == FALSE)
	{
		pContext->bClosing = TRUE;

		// 首先从列表中移除要关闭的连接
		m_lcklstConnection.Lock();
		POSITION pos = m_lstConnection.Find(pContext);
		ASSERT(pos);
		if ( pos)
		{
			m_lstConnection.RemoveAt(pos);
		}
		m_lcklstConnection.Unlock();

		// 然后关闭客户套节字
		pContext->DisconnectServer();
		ReleaseContext(pContext);
	}
	else
	{
		ASSERT(pContext->bClosing == TRUE);
	}
	::LeaveCriticalSection(&pContext->lckSend);
}

void CIOCPServer::CloseAllConnections()
{
	// 遍历整个连接列表，关闭所有的客户套节字
	CTransferChannelEx *pContext = m_lstConnection.GetHead();
	while(pContext != NULL)
	{	
// 		::EnterCriticalSection(&pContext->lckSend);
// 		ASSERT(!pContext->bClosing);
// 		if ( !pContext->bClosing)
// 		{
// 			pContext->bClosing = TRUE;
// 
// 			// 然后关闭客户套节字
// 			pContext->piTrans->Disconnect(pContext);
// 
// 			ASSERT(pContext->pNext == 0);
// 			m_queFreeContext.Push(pContext);
// 		}
// 		::LeaveCriticalSection(&pContext->lckSend);
		CloseAConnection(pContext);
		pContext = m_lstConnection.GetHead();
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
	ASSERT(!m_bServerStarted);
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
				if ( i == 0) //TCP
				{
					// 注册FD_ACCEPT事件。
					// 如果投递的AcceptEx I/O不够，线程会接收到FD_ACCEPT网络事件，说明应该投递更多的AcceptEx I/O
					if ( SOCKET_ERROR == WSAEventSelect(m_TS[0]->GetListenSocket(), m_hAcceptEvent, FD_ACCEPT))
						hr = S_FALSE;	// 有警告,正确返回
				}
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

	ASSERT(m_hListenThread == NULL);
	m_hListenThread = OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());
	if ( !m_hListenThread)
	{
		m_hListenThread = OpenThread(THREAD_SET_CONTEXT, FALSE, GetCurrentThreadId());
	}
	ASSERT(m_hListenThread);

	ASSERT(m_hSyncWorkThread == NULL);
	//m_hSyncWorkThread = (HANDLE)_beginthread(_SyncWorkerThreadProc, 0, this);
	m_hSyncWorkThread = CreateThread(0, 0, _SyncWorkerThreadProc, this, 0, 0);
	ASSERT(m_hSyncWorkThread);
	

	if ( _threadListen() >= 0)
		bResult = TRUE;

End:
	m_bServerStarted = FALSE;

	ASSERT(m_nIocpOutStandingSend == 0);
	ASSERT(m_nIocpOutStandingRecv == 0);
	ASSERT(m_nIocpOutStandingListen == 0);

	return bResult;
}

void CIOCPServer::Shutdown()
{
	if( !m_bServerStarted)
		return;

	// 通知监听线程，马上停止服务
	m_bShutDown = TRUE;
	::SetEvent(m_hAcceptEvent);
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
		//hWaitEvents[nEventCount ++] = (HANDLE)_beginthread(_WorkerThreadProc, 0, this);
		hWaitEvents[nEventCount ++] = ::CreateThread(NULL, 0, _WorkerThreadProc, this, 0, NULL);
	}

	// 下面进入无限循环，处理事件对象数组中的事件

	int nIndex = WSA_WAIT_TIMEOUT;
	while( !m_bShutDown && nIndex != WSA_WAIT_FAILED)
	{
#ifdef _DEBUG
#	define DEF_WAIT_TIMEOUT		15*1000
#else
#	define DEF_WAIT_TIMEOUT		60*1000
#endif // _DEBUG
		nIndex = ::WSAWaitForMultipleEvents(nEventCount, hWaitEvents, FALSE, DEF_WAIT_TIMEOUT, TRUE);
		//nIndex = ::MsgWaitForMultipleObjects(nEventCount, hWaitEvents, FALSE, 60*1000, 0);

		// 检查是否要停止服务
		if( m_bShutDown || nIndex == WSA_WAIT_FAILED)
			break;

		if ( nIndex ==  WSA_WAIT_IO_COMPLETION)	// 0)APC操作执行
		{
			continue;
		}
		else if( nIndex == WSA_WAIT_TIMEOUT)// 1）定时检查所有未返回的AcceptEx I/O的连接建立了多长时间
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

	m_bShutDown = FALSE;

	// 关闭所有应用
	OnShutdown();
	
	// 关闭所有连接
	CloseAllConnections();

	// 关闭线程
	CloseHandle(m_hListenThread);
	m_hListenThread = nullptr;

	SetEvent(m_queSyncRead.m_hEvent);
	CloseHandle(m_hSyncWorkThread);
	m_hSyncWorkThread = nullptr;

	DWORD dwWait = WAIT_IO_COMPLETION ;
	while (dwWait == WAIT_IO_COMPLETION )
		dwWait = ::SleepEx(50, TRUE);		// 给I/O工作线程一个执行的机会

	// 关闭监听套节字
	m_quePendingAccepts.Lock();
	ReadWritePacket* pPacket = m_quePendingAccepts.GetHead();
	while( pPacket != NULL)
	{
		pPacket->piTrans->Disconnect(0, pPacket);
		pPacket = pPacket->pNext;
	}
	m_quePendingAccepts.Unlock();

	dwWait = WAIT_IO_COMPLETION ;
	while (dwWait == WAIT_IO_COMPLETION )
		dwWait = ::SleepEx(50, TRUE);		// 给I/O工作线程一个执行的机会

	for (DWORD i=0;i<m_TransCount;i++)
	{
		if ( m_TS[i])
		{
			m_TS[i]->Final();
		}
	}

	dwWait = WAIT_IO_COMPLETION ;
	while (dwWait == WAIT_IO_COMPLETION )
		dwWait = ::SleepEx(500, TRUE);		// 给I/O工作线程一个执行的机会

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

DWORD CIOCPServer::_threadSyncWorker()
{
#ifdef _DEBUG
	m_log.wprint(L"SyncWorkerThread 启动...");
#endif // _DEBUG

	CTransferChannelEx* pContext = nullptr;
	DWORD dwWaitTime = 1;

	while ( !m_bShutDown)
	{
		pContext = m_queSyncRead.PopAndWait(dwWaitTime);
		if ( !pContext)
		{
			dwWaitTime *= 2;
			if (dwWaitTime > 1000)
				dwWaitTime = 1000;
			continue;
		}

		BOOL bNeedSleep = TRUE;
		DWORD dwCount = m_queSyncRead.GetCount();
		DWORD i = 0;

		do 
		{
			ReadWritePacket* pPacket = AllocateBuffer(pContext, OP_IocpRecv);
			ASSERT(pPacket && pContext->type == SyncMain);
			HRESULT hr = pContext->piTrans->PostRecv(pContext, pPacket);
			if ( S_OK == hr
				 && pPacket->ol.InternalHigh == 0)
			{
				ReleaseBuffer(pPacket);
				m_queSyncRead.PushAndSigned(pContext);

				if (  ++i >= dwCount)
					break;
				pContext = m_queSyncRead.PopAndWait(0);
				continue;
			}

#ifdef _DEBUG
			if ( SUCCEEDED(hr))
			{
				ASSERT(pPacket->ol.InternalHigh > 0);
				CTEPPacket_Header* pHeader = (CTEPPacket_Header*)pPacket->buff.buff;
				ASSERT(pPacket->buff.buff >= pContext->rbufPacket.m_pBuffer
					&& pPacket->buff.buff < pContext->rbufPacket.m_pBuffer + pContext->rbufPacket.m_dwBufferLength);
				ASSERT(pPacket->ol.InternalHigh <= CTEP_DEFAULT_BUFFER_SIZE);
				if ( pPacket->ol.InternalHigh > 7 && pPacket->buff.buff != pContext->rbufPacket.m_pBuffer)
				{
					unsigned char *pStart = (unsigned char *)pPacket->buff.buff - 10 + 2;
					for(unsigned char *p = pStart; p < (unsigned char *)pPacket->buff.buff + 2; p++)
					{
						ASSERT((UCHAR)(p[1] - p[0]) == 1);
					}
				}
				else if (pPacket->buff.buff == pContext->rbufPacket.m_pBuffer)
				{
					ASSERT(pHeader->magic == 'E');
					ASSERT(pHeader->PacketLength <= CTEP_DEFAULT_BUFFER_SIZE);
				}
				else
				{
					ASSERT(pPacket->buff.buff[pPacket->ol.InternalHigh-1] == 0);
				}

				if ( pContext->pParam)
					free(pContext->pParam);

				pContext->pParam = malloc(pPacket->ol.InternalHigh);
				memcpy(pContext->pParam, pPacket->buff.buff, pPacket->ol.InternalHigh);
			}
			else
			{
				ASSERT(pPacket->ol.InternalHigh == 0);
			}
#endif // _DEBUG

// 			pPacket->buff.size = pPacket->ol.InternalHigh;
// 			pPacket->ol.InternalHigh = 0;
// 			int bResult = pContext->PreSplitPacket(pPacket);
// 			if ( bResult == 0)
// 			{
// 				ReleaseBuffer(pPacket);
// 				m_queSyncRead.PushAndSigned(pContext);
// 
// 				if (  ++i >= dwCount)
// 					break;
// 				pContext = m_queSyncRead.PopAndWait(0);
// 				continue;
// 			}

			ASSERT(pContext);
			bNeedSleep = FALSE;
// 			if ( bResult > 0)
// 			{
// 				pPacket->opType = OP_PacketRecv;
// 			}
			BOOL bRet = PostQueuedCompletionStatus(m_hCompletion, pPacket->ol.InternalHigh
				, (ULONG_PTR)pContext, &pPacket->ol);
			ASSERT(bRet);
			if ( !bRet)
			{
				ReleaseBuffer(pPacket);
				CloseAConnection(pContext);
			}
			pContext = nullptr;

			if (  ++i >= dwCount)
				break;

			pContext = m_queSyncRead.PopAndWait(0);
		}
		while ( pContext);

		if ( bNeedSleep)
		{
			dwWaitTime *= 2;
			if (dwWaitTime > 1000)
				dwWaitTime = 1000;
			WaitForSingleObjectEx(m_queSyncRead.m_hEvent, dwWaitTime, true);
		}
		else
		{
			dwWaitTime = 1;
		}
	}

	return 0;
}

DWORD CIOCPServer::_threadIocpWorker()
{
#ifdef _DEBUG
	m_log.wprint(L"	WorkerThread 启动...");
#endif // _DEBUG
	ReadWritePacket *pBuffer;
	DWORD dwTrans;
	LPOVERLAPPED lpol;
	ULONG_PTR dwKey;
	while( !m_bShutDown)
	{
		BOOL bOK = ::GetQueuedCompletionStatus(m_hCompletion, &dwTrans, &dwKey, &lpol, WSA_INFINITE);

		if( dwTrans == (DWORD)-1 && lpol == 0 && dwKey == 0) // 用户通知退出
		{
#ifdef _DEBUG
			ASSERT(bOK);
			m_log.wprint(L"	WorkerThread 退出 \n");
#endif // _DEBUG

			::PostQueuedCompletionStatus(m_hCompletion, (DWORD)-1, 0, NULL);

			DWORD dwWait = WAIT_IO_COMPLETION ;
			while (dwWait == WAIT_IO_COMPLETION )
				dwWait = ::SleepEx(0, TRUE);		// 给I/O工作线程一个执行的机会

			return 0;
		}

		CTransferChannelEx* pContext = (CTransferChannelEx*)dwKey;
		pBuffer = CONTAINING_RECORD(lpol, ReadWritePacket, ol);
		int nError = NO_ERROR;

		ASSERT(dwTrans == pBuffer->ol.InternalHigh || dwTrans == 0);
		if ( pBuffer->piTrans == m_TS[0] || pBuffer->piTrans == m_TS[1])	// TCP | UDP
		{
			DWORD dwFlags = 0;
			SOCKET s;
			if ( pContext)
			{
				s = pContext->s;
				ASSERT(pBuffer->opType == EmPacketOperationType::OP_IocpRecv || pBuffer->opType == EmPacketOperationType::OP_IocpSend);
			}
			else
			{
				ASSERT(pBuffer->opType == EmPacketOperationType::OP_Listen);
				s = pBuffer->piTrans->GetListenSocket();
			}

			bOK = WSAGetOverlappedResult(s, &pBuffer->wsaol, &dwTrans, FALSE, &dwFlags);
			if ( !bOK)
			{
				nError = WSAGetLastError();
			}
		}
		else if ( pContext && pContext->type != SyncMain)	// 其他Iocp传输
		{
			bOK = GetOverlappedResult(pContext->hFile, &pBuffer->ol, &dwTrans, FALSE);
			if ( !bOK)
			{
				nError = GetLastError();
			}
		}
		else	// 同步传输, RDP/ICA
		{
			dwTrans = pBuffer->ol.InternalHigh;
			nError = pBuffer->ol.Internal;
			if ( nError != 0)
			{
				dwTrans = 0;
				bOK = FALSE;
			}
		}

		if ( !bOK || nError != 0)
		{
			ASSERT(!bOK && nError != 0);
			m_log.FmtWarning(3, L"在此套节字上有错误发生(%x)! pContext:0x%08x dwTrans:%d ErrCode:%d"
				,pBuffer->hFile, pContext, dwTrans, nError);
			ASSERT(nError !=  WSA_IO_INCOMPLETE && nError != ERROR_IO_PENDING);
			if ( nError == 0)
				nError = -1;
			dwTrans = 0;
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

void CIOCPServer::HandleIO(CTransferChannelEx *pContext, ReadWritePacket *pBuffer, DWORD dwTrans, int nError)
{
	EmPacketOperationType ePacketType = pBuffer->opType;
	if ( pContext)
	{
		if ( pContext->type != SyncMain)
		{
			if( ePacketType == EmPacketOperationType::OP_IocpRecv)
			{
				InterlockedDecrement(&m_nIocpOutStandingRecv);
			}
			else if( ePacketType == EmPacketOperationType::OP_IocpSend)
			{
				InterlockedDecrement(&pContext->nOutstandingSend);

#if !CTEP_DEBUG_SENDPACKET_LEVEL_OFF
				InterlockedDecrement(&m_nIocpOutStandingSend);
#endif
			}
		}

		if( pContext->bClosing)
		{
			m_log.FmtMessage(1, L"	检查到套节字已经被我们关闭 \n");

			ReleaseBuffer(pBuffer);
			return;
		}
	}
	else
	{
#ifdef _DEBUG
		ASSERT( pBuffer->opType == OP_Listen && pBuffer->buff.size == 0);
		ASSERT( dwTrans == sizeof(CTEPPacket_Init) || dwTrans == 0);
#endif // _DEBUG
		RemovePendingAccept(pBuffer);// Listen操作, 出等待列表
#ifdef _DEBUG
		ASSERT( pBuffer->opType == OP_Listen && pBuffer->buff.size == 0);
		ASSERT( dwTrans == sizeof(CTEPPacket_Init) || dwTrans == 0);
#endif // _DEBUG
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
		}
		else
		{ // 在监听套节字上发生错误，也就是监听套节字处理的客户出错了
			if ( pBuffer->piTrans)
			{
				pBuffer->piTrans->Disconnect(0, pBuffer);
			}
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
		ASSERT(pBuffer->hFile != INVALID_HANDLE_VALUE);
		if( pBuffer->hFile != INVALID_HANDLE_VALUE)
		{
			// 为新接受的连接申请客户上下文对象
			CTransferChannelEx *pClient = AllocateContext(pBuffer->hFile, pBuffer->piTrans);
			BOOL bSuccess = FALSE;
			if(pClient)
			{
				if( AddAConnection(pClient))
				{
					do 
					{
						//SetFileCompletionNotificationModes(pBuffer->hFile, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS|FILE_SKIP_SET_EVENT_ON_HANDLE);
						pClient->piTrans->CompleteListen(pClient, pBuffer);
						if ( pClient->type == TCP || pClient->type == UDP || pClient->type == IocpMain)
						{
							// 关联新连接到完成端口对象
							HANDLE hIocp = ::CreateIoCompletionPort(pClient->hFile, m_hCompletion, (DWORD)pClient, 0);
							if ( hIocp != m_hCompletion)
							{
								ASSERT(hIocp == m_hCompletion);
								break;
							}
						}

						// 通知用户
						pClient->bNeedNotificationClosing = TRUE;
						OnConnectionEstablished(pClient, pBuffer);

						// 向新连接投递1个Read请求，这些空间在套节字关闭或出错时释放
						if ( pClient->type == SyncMain)
						{
							m_queSyncRead.PushAndSigned(pClient);
							bSuccess = TRUE;
						}
						else
						{
							ASSERT(pClient->type == TCP || pClient->type == UDP);
							ReadWritePacket* pRecv = AllocateBuffer(pClient, OP_IocpRecv);
							HRESULT hr = pClient->piTrans->PostRecv(pClient, pRecv);
							if ( pRecv && (hr == S_OK || hr == ERROR_IO_PENDING))
							{
								InterlockedIncrement(&m_nIocpOutStandingRecv);
								bSuccess = TRUE;
							}
							else
							{
								ASSERT(0);
							}
						}

					} while (0);
				}
				else
				{
					ASSERT(0);
				}
				
				if ( !bSuccess)
				{
					if ( InterlockedExchange((volatile UINT*)&pClient->bNeedNotificationClosing, FALSE))
					{
						OnConnectionClosing(pClient, 0);
					}
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
	else if ( pBuffer->opType == EmPacketOperationType::OP_PacketRecv)
	{
		ASSERT( pContext->type == SyncMain);
		OnReadCompleted(pContext, pBuffer);// 通知用户
		ReleaseBuffer(pBuffer);
	}
	else if(pBuffer->opType == EmPacketOperationType::OP_IocpRecv)
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
	else if(pBuffer->opType == EmPacketOperationType::OP_IocpSend)
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

	if ( pContext && !pContext->bClosing && ePacketType == EmPacketOperationType::OP_IocpRecv)
	{
		// 继续投递一个新的接收请求
		if ( pContext->type == SyncMain)
		{
			m_queSyncRead.PushAndSigned(pContext);
		}
		else
		{
			ASSERT(pContext->type == TCP || pContext->type == UDP);
			ReadWritePacket* pRecv = AllocateBuffer(pContext, OP_IocpRecv);
			HRESULT hr = pContext->piTrans->PostRecv(pContext, pRecv);
			if ( pRecv && (hr == S_OK || hr == ERROR_IO_PENDING))
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

// BOOL CIOCPServer::SendText(StTransferChannelEx *pContext, char *pszText, int nLen, EmPacketLevel level)
// {
// 	// 跟踪投递的发送的数量，防止用户仅发送数据而不接收，导致服务器抛出大量发送操作
// 	if(pContext->nOutstandingSend > m_nMaxSends)
// 	{
// 		m_log.FmtWarningW(3, L"PostSend overflow! %d > max(%d)"
// 			, pContext->nOutstandingSend, m_nMaxSends);
// 		ASSERT(0);
// 		return FALSE;
// 	}
// 
// 	ReadWritePacket *pBuffer = AllocateBuffer((StTransferChannel*)pContext, OP_IocpSend, nLen);
// 	if ( !pBuffer)
// 		return FALSE;
// 
// 	memcpy(pBuffer->buff.buff, pszText, nLen);
// 
// 	if ( m_mqueSendPriority.Push(pBuffer, level))
// 		return TRUE;
// 
// 	return sendMQUEPacket(pBuffer);
// }

BOOL CIOCPServer::SendPacket(CTransferChannelEx *pContext, ReadWritePacket* pBuffer, EmPacketLevel level)
{
	BOOL bResult = TRUE;
	if ( !pContext)
	{
		ReleaseBuffer(pBuffer);
		return FALSE;
	}

	// 跟踪投递的发送的数量，防止用户仅发送数据而不接收，导致服务器抛出大量发送操作
	int nOutstandingSend = pContext->nOutstandingSend;
	if ( nOutstandingSend > 500)
	{
		SleepEx(0, TRUE);
	}

	if( nOutstandingSend > m_nMaxSends)
	{
		m_log.FmtErrorW(3, L"PostSend overflow! %d > max(%d)", nOutstandingSend, m_nMaxSends);
	}
	
	::EnterCriticalSection(&pContext->lckSend);
#if !CTEP_DEBUG_SENDPACKET_LEVEL_OFF
	if ( m_mqueSendPriority.Push(pBuffer, level))
	{
		::LeaveCriticalSection(&pContext->lckSend);
		return TRUE;
	}
#endif

	bResult = sendMQUEPacket(pBuffer);
	::LeaveCriticalSection(&pContext->lckSend);

	return bResult;
}

BOOL CIOCPServer::sendMQUEPacket(ReadWritePacket* pSend)
{
	HRESULT hr;
	CTransferChannelEx *pCtxSend = (CTransferChannelEx*)pSend->pTransChn;
	ASSERT(pCtxSend && pSend->opType == EmPacketOperationType::OP_IocpSend);
	if ( !pCtxSend || pCtxSend->bClosing)
	{
		ReleaseBuffer(pSend);
		return FALSE;
	}

	if ( pCtxSend->type != SyncMain)
	{
		// 套节字上的未决I/O计数
		DWORD dwOutstandingSend = InterlockedIncrement(&pCtxSend->nOutstandingSend);
#if !CTEP_DEBUG_SENDPACKET_LEVEL_OFF
		DWORD dwIocpOsSend = InterlockedIncrement(&m_nIocpOutStandingSend);
		ASSERT ( dwIocpOsSend < MQUE_MAX_SEND + 15 && dwOutstandingSend < MQUE_MAX_SEND + 15);
#endif
	}

	::EnterCriticalSection(&pCtxSend->lckSend);
	DWORD SequenceIdLocal = InterlockedIncrement(&pCtxSend->SequenceIdLocal);
	((CTEPPacket_Header*)pSend->buff.buff)->SequenceId = SequenceIdLocal;
	hr = pCtxSend->SendPacketServer(pSend);
	::LeaveCriticalSection(&pCtxSend->lckSend);

	if ( S_OK == hr || hr == ERROR_IO_PENDING)
	{
		if ( pCtxSend->type != SyncMain)
		{
			OnWriteCompleted(0, 0);
		}
		else
		{
			ReleaseBuffer(pSend);
		}

		return TRUE;
	}

	if ( pCtxSend->type != SyncMain)
	{
		// 套节字上的未决I/O计数
#if !CTEP_DEBUG_SENDPACKET_LEVEL_OFF
		InterlockedDecrement(&m_nIocpOutStandingSend);
#endif
		InterlockedDecrement(&pCtxSend->nOutstandingSend);
	}

	m_log.FmtWarning(5, L"sendMQUEPacket-PostSend Failed. SendSize:(%d) hr:0x%08x(%d)"
		, pSend->buff.size, hr, hr);
	CloseAConnection(pCtxSend);
	ReleaseBuffer(pSend);

	return FALSE;
}


void CIOCPServer::OnWriteCompleted(CTransferChannelEx * /*pContext*/, ReadWritePacket * /*pBuffer*/)
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

