// CTEPTSCtvp.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "CTEPTSCross.h"

#pragma warning(disable:4482)//warning C4482: 使用了非标准扩展: 限定名中使用了枚举“EmPacketOperationType”

CTransSvrCrossApp gOne;

ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServerCrossApp()
{
	return dynamic_cast<ICTEPTransferProtocolServer*>(&gOne);
}

HRESULT CTransSvrCrossApp::InitializeCompletePort(ICTEPTransferProtocolCallBack* piCallBack)
{
	ASSERT(piCallBack);
	ASSERT( !m_piCallBack || m_piCallBack == piCallBack);
	if ( piCallBack)
	{
		if ( m_piCallBack == piCallBack)
		{
			return S_FALSE;
		}
		else if ( !m_piCallBack)
		{
			m_piCallBack = piCallBack;
			return S_OK;
		}
		else
		{
			return E_INVALIDARG;
		}
	}

	return E_UNEXPECTED;
}

HRESULT CTransSvrCrossApp::PostListen(bool bFirst )
{
	if ( bFirst)
		return m_TrdPipeMonitor.StartThread();


	ASSERT( m_TrdPipeMonitor.IsWorking());

	if ( m_TrdPipeMonitor.IsWorking())
		return S_OK;
	
	return E_FAIL;
}
HRESULT CTransSvrCrossApp::PostRecv(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	if ( !m_piCallBack)
	{
		ASSERT(0);
		return E_FAIL;
	}
	if( !pPacket)
		return E_FAIL;

	// 设置I/O类型
	ASSERT(pPacket->opType == EmPacketOperationType::OP_IocpRecv);

	// 投递此重叠I/O
	DWORD dwByte = 0;
	if( ReadFile(pTransChn->hFile, pPacket->buff.buff, pPacket->buff.maxlength, &dwByte, &pPacket->ol) == FALSE)
	{
		DWORD dwErr = GetLastError();
		if( dwErr != ERROR_IO_PENDING)
		{
			m_log.FmtErrorW(3, L"PostSend failed. ErrCode:%d", dwErr);
			return MAKE_WINDOWS_ERRCODE(dwErr);
		}

		return ERROR_IO_PENDING;
	}

	ASSERT(pPacket->ol.InternalHigh == dwByte);
	return S_OK;
}
HRESULT CTransSvrCrossApp::PostSend(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
#ifdef _DEBUG
	USHORT* pCountPacket = (USHORT*)pPacket->buff.buff;
	ASSERT(*pCountPacket == pPacket->buff.size);
	ASSERT(pPacket->opType == EmPacketOperationType::OP_IocpSend);
#endif // _DEBUG
	pPacket->opType = EmPacketOperationType::OP_IocpSend;
	// 投递此重叠I/O
	DWORD dwBytes;
	if (FALSE == WriteFile(pTransChn->hFile,pPacket->buff.buff,pPacket->buff.size,&dwBytes,&pPacket->ol))
	{
		long dwErr = GetLastError();
		if( dwErr != ERROR_IO_PENDING)
		{
			ASSERT(dwErr > 0);
			m_log.FmtErrorW(3, L"PostSend failed. ErrCode:%d", dwErr);
			return 0-dwErr;
		}

		return ERROR_IO_PENDING;
	}

	ASSERT(dwBytes == pPacket->buff.size);
	return S_OK;
}

HRESULT CTransSvrCrossApp::CompleteListen(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	ASSERT(pPacket->opType == EmPacketOperationType::OP_Listen);
	//取得客户端管道句柄,在监听连接成功后，将客户端的句柄保存在ReadWritePacket中 hFile
	pTransChn->type = EnTransferChannelType::TransType_CrossApp;
	ASSERT(pPacket->hFile != INVALID_HANDLE_VALUE);
	if (pPacket->hFile == INVALID_HANDLE_VALUE)
		return E_FAIL;

	ASSERT(pPacket->ol.hEvent);
	CloseHandle(pPacket->ol.hEvent);
	pPacket->ol.hEvent = nullptr;

	pTransChn->hFile = pPacket->hFile;
	return S_OK;
}

HRESULT CTransSvrCrossApp::Disconnect(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	if ( pPacket && pTransChn)
	{
		ASSERT(pTransChn->hFile == pPacket->hFile);
	}

	if( pPacket)
	{
		ASSERT( pPacket->opType == EmPacketOperationType::OP_Listen);
		CloseHandle(pPacket->hFile);
		pPacket->hFile = INVALID_HANDLE_VALUE;
	}
	if ( pTransChn)
	{
		CloseHandle(pTransChn->hFile);
		pTransChn->hFile = INVALID_HANDLE_VALUE;
	}

	if ( !pPacket && !pTransChn)
	{
		ASSERT(0);
		return E_FAIL;
	}

	return S_OK;
}

void CTransSvrCrossApp::Final()
{
	m_TrdPipeMonitor.StopThread();
}

DWORD CTransSvrCrossApp::_MonitorPipeThread(LPVOID param, HANDLE hEventMessage, CWorkerThread<CTransSvrCrossApp>* pWrkTrd)
{
	HRESULT hr = S_OK;
	DWORD dwError = 0;
	OVERLAPPED ol = {0};

	HANDLE hMmrPipe = INVALID_HANDLE_VALUE;
	ol.hEvent = CreateEvent(0, TRUE, FALSE, 0);

	while ( pWrkTrd->IsWorking())
	{
		hMmrPipe = INVALID_HANDLE_VALUE;
		SECURITY_ATTRIBUTES* pSA = 0;

#if SVRNAMEDPIPE_LOW_SECURITY_PIPE
		pSA = CUserPrivilege::CreateLowIntegritySecurityAttributes(FALSE);
#endif // SVRNAMEDPIPE_LOW_SECURITY_PIPE

		hMmrPipe = CreateNamedPipe(CTEPTS_CROSSAPP_PIPE_NAME_TEMPLATE
			, PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED
			, PIPE_TYPE_MESSAGE
			, PIPE_UNLIMITED_INSTANCES
			, CTEP_DEFAULT_BUFFER_SIZE
			, CTEP_DEFAULT_BUFFER_SIZE
			, NMPWAIT_USE_DEFAULT_WAIT
			, pSA
			);
		dwError = GetLastError();

#if SVRNAMEDPIPE_LOW_SECURITY_PIPE
		CUserPrivilege::DeleteLowIntegritySecurityAttributes(pSA);
#endif // SVRNAMEDPIPE_LOW_SECURITY_PIPE

		if ( hMmrPipe  == INVALID_HANDLE_VALUE)
		{
			ASSERT(0);
			hr = MAKE_WINDOWS_ERRCODE(dwError);
			goto ErrorEnd;
		}

		ClearOverlaped(&ol);
		BOOL bConnect = ConnectNamedPipe(hMmrPipe, &ol);
		dwError = GetLastError();
		if ( !bConnect && dwError != ERROR_IO_PENDING && dwError != ERROR_PIPE_CONNECTED)
		{
			ASSERT(dwError == ERROR_BROKEN_PIPE && !pWrkTrd->IsWorking());	// 管道已经关闭,程序等待退出
			if ( !pWrkTrd->IsWorking())
				break;

			m_log.FmtWarningW(2, L"创建连入的管道失败. 错误代码: %d", dwError);
			continue;
		}

		HANDLE hGroup[] = {hEventMessage, ol.hEvent};
		DWORD dwWait = pWrkTrd->MsgWait(hGroup, 2, INFINITE);
		if ( dwWait == 0)
		{
			ASSERT(!pWrkTrd->IsWorking());
			break;
		}
		else
		{
			ASSERT(dwWait == 1);
		}
		DWORD dwRead = 0;
		bConnect = GetOverlappedResult(hMmrPipe, &ol, &dwRead, FALSE);
		if ( !bConnect)
		{
			dwError = GetLastError();
			m_log.FmtWarningW(2, L"一个连入的管道发生错误. 错误代码:%d", dwError);
			continue;
		}

		// 获取第一个包,确认用户已经连入了
		ReadWritePacket *m_Rwpacket = m_piCallBack->AllocatePacket(CTEPGetInterfaceTransServerCrossApp());
		m_Rwpacket->ol.hEvent = CreateEvent(0, TRUE, 0, 0);
		ASSERT(m_Rwpacket->ol.hEvent && m_Rwpacket->buff.buff && m_Rwpacket->buff.maxlength);
		BOOL bRead = ReadFile(hMmrPipe
			, m_Rwpacket->buff.buff
			, m_Rwpacket->buff.maxlength
			, &dwRead
			, &m_Rwpacket->ol);
		dwError = GetLastError();
		if ( bRead || dwError == ERROR_IO_PENDING)
		{
			bRead = GetOverlappedResult(hMmrPipe, &m_Rwpacket->ol, &dwRead, TRUE);
		}

		ASSERT(bRead && dwRead > 0);
		if ( !bRead || dwRead == 0)
		{
			ASSERT(dwError == ERROR_PIPE_NOT_CONNECTED	// 管道被关闭
				|| dwError == ERROR_BROKEN_PIPE);	// 管道被关闭
			CloseHandle(m_Rwpacket->ol.hEvent);
			m_Rwpacket->ol.hEvent = NULL;
			m_piCallBack->FreePacket(m_Rwpacket);
			CloseHandle(hMmrPipe);
			hMmrPipe = INVALID_HANDLE_VALUE;
			continue;
		}

		// 通知上层一个新的连接建立
		m_Rwpacket->hFile = hMmrPipe;
		m_Rwpacket->opType = EmPacketOperationType::OP_Listen;
		if( !PostQueuedCompletionStatus(m_piCallBack->GetCompletePort(), dwRead, 0, &m_Rwpacket->ol))
		{
			ASSERT(0);
			CloseHandle(m_Rwpacket->hFile);
		}
	}

	CloseHandle(ol.hEvent);

	return 0;

ErrorEnd:
	CloseHandle(hMmrPipe);
	CloseHandle(ol.hEvent);
	pWrkTrd->SetErrorStop(hr);

	return -1;
}




