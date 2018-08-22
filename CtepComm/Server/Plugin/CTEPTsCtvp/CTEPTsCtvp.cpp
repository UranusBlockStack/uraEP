// CTEPTSCtvp.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "CTEPTSCtvp.h"
CTransProCtvpSvr gOne;

BOOL CTransProCtvpSvr::ctepTsCtvpMainThreadInit()
{
	LOCK(&m_lck);
	if( h_MonitorThread != NULL)
		return TRUE;

	//启动监听线程
	h_MonitorThread = CreateThread(NULL, 0, _trdMonitorPipeThread, this, 0, 0);

	return (BOOL)h_MonitorThread;
}

void CTransProCtvpSvr::ctepTsCtvpMainThreadClose()
{
	LOCK(&m_lck);
	if( h_MonitorThread != NULL)
	{
		HANDLE hTemp = h_MonitorThread;
		h_MonitorThread = NULL;
		DWORD dwWait = WaitForSingleObject(hTemp, 5000);
		if ( dwWait == WAIT_TIMEOUT)
		{
			ASSERT(0);
			TerminateThread(hTemp, (DWORD)-199);
		}
	}
}


ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServer()
{
	return dynamic_cast<ICTEPTransferProtocolServer*>(&gOne);
}

HRESULT CTransProCtvpSvr::InitializeCompletePort(ICTEPTransferProtocolCallBack* piCallBack)
{
	ASSERT(piCallBack);
	ASSERT(m_piCallBack == NULL || m_piCallBack == piCallBack);
	ASSERT(m_hMmrPipe == INVALID_HANDLE_VALUE);

	if ( piCallBack)
	{
		if ( m_piCallBack)
		{
			m_piCallBack = piCallBack;
			return S_FALSE;
		}
		
		m_piCallBack = piCallBack;
		return S_OK;
	}

	return E_UNEXPECTED;
}

HRESULT CTransProCtvpSvr::PostListen(bool bFirst )
{
	HRESULT hr = E_FAIL;
	if ( h_MonitorThread == NULL)
	{
		if( ctepTsCtvpMainThreadInit())
			hr = S_OK;
	}
	else
	{
		hr = S_FALSE;
	}

	return hr;
}
HRESULT CTransProCtvpSvr::PostRecv(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
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
		if( dwErr != WSA_IO_PENDING)
		{
			ASSERT(dwErr > 0);
			m_log.FmtErrorW(3, L"PostSend failed. ErrCode:%d", dwErr);
			return 0-dwErr;
		}
		else
		{
			return ERROR_IO_PENDING;
		}
	}
	ASSERT(pPacket->ol.InternalHigh == dwByte);
	return S_OK;
}
HRESULT CTransProCtvpSvr::PostSend(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
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
		if( dwErr != WSA_IO_PENDING)
		{
			ASSERT(dwErr > 0);
			m_log.FmtErrorW(3, L"PostSend failed. ErrCode:%d", dwErr);
			return 0-dwErr;
		}
		else
		{
			return ERROR_IO_PENDING;
		}
	}
	ASSERT(dwBytes == pPacket->buff.size);
	return S_OK;
}

HRESULT CTransProCtvpSvr::CompleteListen(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	ASSERT(pPacket->opType == EmPacketOperationType::OP_Listen);
	//取得客户端管道句柄,在监听连接成功后，将客户端的句柄保存在ReadWritePacket中 hFile
	pTransChn->type = EnTransferChannelType::TransType_IocpMain;
	ASSERT(pPacket->hFile != INVALID_HANDLE_VALUE);
	if (pPacket->hFile == INVALID_HANDLE_VALUE)
		return E_FAIL;

	pTransChn->hFile = pPacket->hFile;
	return S_OK;
}

HRESULT CTransProCtvpSvr::Disconnect(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	if ( pPacket && pTransChn)
	{
		ASSERT(pTransChn->hFile == pPacket->hFile);
	}
	if ( m_hMmrPipe)
	{
		CloseHandle(m_hMmrPipe);
		m_hMmrPipe = INVALID_HANDLE_VALUE;
	}
	if( pPacket)
	{
		ASSERT( pPacket->opType == EmPacketOperationType::OP_Listen);
		pPacket->hFile = INVALID_HANDLE_VALUE;
	}
	if ( pTransChn)
	{
		pTransChn->hFile = INVALID_HANDLE_VALUE;
	}

	if ( !pPacket && !pTransChn)
	{
		ASSERT(0);
		return E_FAIL;
	}

	return S_OK;
}

void CTransProCtvpSvr::Final()
{
	if (m_hMmrPipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hMmrPipe);
		m_hMmrPipe = INVALID_HANDLE_VALUE;
	}

	ctepTsCtvpMainThreadClose();
}


DWORD CTransProCtvpSvr::_MonitorPipeThread()
{
	DWORD dwError = 0;
	while ( h_MonitorThread)
	{
		DWORD dwConsoleId = WTSGetActiveConsoleSessionId();
		if ( dwConsoleId == (DWORD)-1)
		{
			SleepEx(1000, TRUE);
			continue;
		}
		WCHAR PipeName[MAX_PATH];
		swprintf_s(PipeName, CTEPTS_CTVP_PIPE_NAME_TEMPLATE, dwConsoleId);
		BOOL bWait = WaitNamedPipe(PipeName, NMPWAIT_WAIT_FOREVER);
		if ( !bWait)
		{
			if (h_MonitorThread)
				SleepEx(15000, TRUE);	// 如果没有找到指定Pipe管道,则等待15秒后再试
			continue;
		}

		m_hMmrPipe = CreateFile(PipeName,
			GENERIC_READ|GENERIC_WRITE, 
			0,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED,
			NULL);
		dwError = GetLastError();
		if ( m_hMmrPipe  == INVALID_HANDLE_VALUE)
		{
			ASSERT(dwError == ERROR_PIPE_BUSY);
			continue;
		}

		// 获取第一个包,确认用户已经连入了
		ReadWritePacket *m_Rwpacket = m_piCallBack->AllocatePacket(CTEPGetInterfaceTransServer());
		m_Rwpacket->ol.hEvent = CreateEvent(0, TRUE, 0, 0);
		ASSERT(m_Rwpacket->ol.hEvent && m_Rwpacket->buff.buff && m_Rwpacket->buff.maxlength);
		DWORD dwRead = 0;
		BOOL bRead = ReadFile(m_hMmrPipe, m_Rwpacket->buff.buff, m_Rwpacket->buff.maxlength
			, &dwRead, &m_Rwpacket->ol);
		if ( !bRead)
		{
			bRead = GetOverlappedResult(m_hMmrPipe, &m_Rwpacket->ol, &dwRead, TRUE);
			dwError = GetLastError();
		}

		if ( !bRead || dwRead == 0)
		{
			ASSERT(dwError == ERROR_PIPE_NOT_CONNECTED	// 管道被关闭
				|| dwError == ERROR_BROKEN_PIPE);	// 管道被关闭
			CloseHandle(m_Rwpacket->ol.hEvent);
			m_Rwpacket->ol.hEvent = NULL;
			m_piCallBack->FreePacket(m_Rwpacket);
			CloseHandle(m_hMmrPipe);
			m_hMmrPipe = INVALID_HANDLE_VALUE;
			continue;
		}

		// 通知上层一个新的连接建立
		CloseHandle(m_Rwpacket->ol.hEvent);
		m_Rwpacket->ol.hEvent = nullptr;
		m_Rwpacket->hFile = m_hMmrPipe;
		m_Rwpacket->opType = EmPacketOperationType::OP_Listen;
		if( !PostQueuedCompletionStatus(m_piCallBack->GetCompletePort(), dwRead, 0, &m_Rwpacket->ol))
		{
			ASSERT(0);
			break;
		}
	}

	return 0;
}




