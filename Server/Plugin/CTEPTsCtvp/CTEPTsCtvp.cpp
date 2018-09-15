// CTEPTSCtvp.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "CTEPTSCtvp.h"

#pragma warning(disable:4482)//warning C4482: 使用了非标准扩展: 限定名中使用了枚举“EmPacketOperationType”

CTransProCtvpSvr gOne;

#define SetBuffEndDwordKey(pBuff, nSize, dwKey)		( *(DWORD*)( ((char*)pBuff)+(nSize - sizeof(DWORD)) ) = (dwKey) )
#define GetBuffEndDwordKey(pBuff, nSize)			( *(DWORD*)( ((char*)pBuff)+(nSize - sizeof(DWORD)) ) )

ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServer()
{
	return dynamic_cast<ICTEPTransferProtocolServer*>(&gOne);
}

HRESULT CTransProCtvpSvr::InitializeCompletePort(ICTEPTransferProtocolCallBack* piCallBack)
{
	ASSERT(piCallBack);
	ASSERT(m_piCallBack == NULL || m_piCallBack == piCallBack);
	ASSERT(m_hPipeCurrCtvp == INVALID_HANDLE_VALUE);

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
	if ( !m_Worker.IsWorking())
	{
		if( m_Worker.StartThread())
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
			return MAKE_WINDOWS_ERRCODE(dwErr);
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
			return MAKE_WINDOWS_ERRCODE(dwErr);
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

	ASSERT( pPacket->buff.buff && pPacket->buff.size > 0 && pPacket->buff.maxlength >= pPacket->buff.size + 4);
	if ( !pPacket->buff.buff || pPacket->buff.size <= 0 || pPacket->buff.maxlength < pPacket->buff.size + 4)
		return E_INVALIDARG;

	DWORD dwSessionId = GetBuffEndDwordKey(pPacket->buff.buff, pPacket->buff.maxlength);
	pTransChn->dwSessionId = dwSessionId;
	pTransChn->hFile = pPacket->hFile;
	return S_OK;
}

HRESULT CTransProCtvpSvr::Disconnect(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	if ( pPacket && pTransChn)
	{
		ASSERT(pTransChn->hFile == pPacket->hFile);
	}

	if( pPacket)
	{
		ASSERT( pPacket->opType == EmPacketOperationType::OP_Listen);
		ASSERT(pPacket->hFile == m_hPipeCurrCtvp);
		pPacket->hFile = INVALID_HANDLE_VALUE;
	}
	if ( pTransChn)
	{
		ASSERT(pTransChn->hFile == m_hPipeCurrCtvp);
		pTransChn->hFile = INVALID_HANDLE_VALUE;
	}

	if ( m_hPipeCurrCtvp)
	{
		CloseHandle(m_hPipeCurrCtvp);
		m_hPipeCurrCtvp = INVALID_HANDLE_VALUE;

		CloseHandle(m_hPipeWaitConnect);
		m_Worker.SetEvent();
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
	CloseHandle(m_hPipeWaitConnect);
	CloseHandle(m_hPipeCurrCtvp);
	m_hPipeCurrCtvp = m_hPipeWaitConnect = INVALID_HANDLE_VALUE;

	m_Worker.StopThread();
}


DWORD CTransProCtvpSvr::_MonitorPipeThread(LPVOID param, HANDLE hEventMessage, MyWorker* pWrkTrd)
{
	DWORD dwError = 0;
	m_log.Message(1, L"_MonitorPipeThread in.");
	while ( pWrkTrd->IsWorking())
	{
		DWORD dwConsoleId = WTSGetActiveConsoleSessionId();
		if ( dwConsoleId == (DWORD)-1)
		{
			//SleepEx(1000, TRUE);
			//continue;
			dwConsoleId = 0;
		}
		WCHAR PipeName[MAX_PATH];
		swprintf_s(PipeName, CTEPTS_CTVP_PIPE_NAME_TEMPLATE, dwConsoleId);
		BOOL bWait = WaitNamedPipe(PipeName, 3000);
		if ( !bWait)
		{
			dwError = GetLastError();
			m_log.FmtMessage(1, L"_MonitorPipeThread - WaitNamedPipe() failed. Pipe:[%s] ErrCode:%d"
				, PipeName, dwError);
			swprintf_s(PipeName, CTEPTS_CTVP_PIPE_NAME_TEMPLATE, 0);
			bWait = WaitNamedPipe(PipeName, 0);
		}
		if ( !bWait)
		{
			pWrkTrd->WaitEvent(1000);	// 如果没有找到指定Pipe管道,则等待5秒后再试
			continue;
		}

		m_hPipeWaitConnect = CreateFile(PipeName,
			GENERIC_READ|GENERIC_WRITE, 
			0,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED,
			NULL);
		dwError = GetLastError();
		if ( m_hPipeWaitConnect  == INVALID_HANDLE_VALUE)
		{
			ASSERT(dwError == ERROR_PIPE_BUSY);
			m_log.FmtError(1, L"_MonitorPipeThread CreateFile() failed. Pipe:[%s] ErrCode:%d"
				, PipeName, dwError);
			continue;
		}

		m_log.FmtMessage(2, L"_MonitorPipeThread New Link in. handle:0x%08x, pipe:[%s]"
			, m_hPipeWaitConnect, PipeName);
		// 获取第一个包,确认用户已经连入了
		ReadWritePacket *m_Rwpacket = m_piCallBack->AllocatePacket(CTEPGetInterfaceTransServer());
		m_Rwpacket->ol.hEvent = CreateEvent(0, TRUE, 0, 0);
		SetBuffEndDwordKey(m_Rwpacket->buff.buff, m_Rwpacket->buff.maxlength, dwConsoleId);

		ASSERT(m_Rwpacket->ol.hEvent && m_Rwpacket->buff.buff && m_Rwpacket->buff.maxlength > sizeof(DWORD)+sizeof(CTEPPacket_Header));
		DWORD dwRead = 0;
		BOOL bRead = ReadFile(m_hPipeWaitConnect, m_Rwpacket->buff.buff, m_Rwpacket->buff.maxlength - sizeof(DWORD)
			, &dwRead, &m_Rwpacket->ol);
		dwError = GetLastError();
		if ( bRead || dwError == ERROR_IO_PENDING)
		{
			HANDLE hGroup[] = {hEventMessage, m_Rwpacket->ol.hEvent};
			DWORD dwWait = pWrkTrd->MsgWait(hGroup, 2, INFINITE);
			ResetEvent(hEventMessage);
			bRead = GetOverlappedResult(m_hPipeWaitConnect, &m_Rwpacket->ol, &dwRead, FALSE);
			dwError = GetLastError();
			if ( dwError == ERROR_IO_INCOMPLETE)
			{
				ASSERT(dwWait == 0);
			}
		}

		if ( !bRead || dwRead == 0)
		{
			m_log.FmtError(2, L"_MonitorPipeThread New Link ReadFile() First Bag failed. handle:0x%08x ErrCode:%d"
				, m_hPipeWaitConnect, dwError);
			ASSERT(dwError == ERROR_PIPE_NOT_CONNECTED	// 管道被关闭
				|| dwError == ERROR_BROKEN_PIPE			// 管道被关闭
				|| dwError == ERROR_IO_INCOMPLETE);
			CloseHandle(m_Rwpacket->ol.hEvent);
			m_Rwpacket->ol.hEvent = NULL;
			m_piCallBack->FreePacket(m_Rwpacket);
			CloseHandle(m_hPipeWaitConnect);
			continue;
		}

		m_log.FmtMessage(2, L"_MonitorPipeThread New Link First Bag. Size:%d", dwRead);

		// 通知上层一个新的连接建立
		CloseHandle(m_Rwpacket->ol.hEvent);
		m_Rwpacket->ol.hEvent = nullptr;
		m_hPipeCurrCtvp = m_hPipeWaitConnect;
		m_Rwpacket->hFile = m_hPipeWaitConnect;
		m_Rwpacket->opType = EmPacketOperationType::OP_Listen;
		if( !PostQueuedCompletionStatus(m_piCallBack->GetCompletePort(), dwRead, 0, &m_Rwpacket->ol))
		{
			ASSERT(0);
			break;
		}
	}

	m_log.Message(1, L"_MonitorPipeThread Out.");
	return 0;
}




