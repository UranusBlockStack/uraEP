// CTEPTStestTcp.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "CTEPTSTcp.h"

CTransProTcpSvr gOne;

ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServer()
{
	return dynamic_cast<ICTEPTransferProtocolServer*>(&gOne);
}

CTransProTcpSvr::CTransProTcpSvr()
	: m_piCallBack(0),
	m_sListen(INVALID_SOCKET),
	m_lpfnGetAcceptExSockaddrs(NULL),
	m_lpfnAcceptEx(NULL),
	m_log("TS_TCP"),
	m_uPort(DEFAULT_PORT)
{
	// 初始化WS2_32.dll
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(2, 2);
	::WSAStartup(sockVersion, &wsaData);
	InitializeCriticalSection(&cs);
}
CTransProTcpSvr::~CTransProTcpSvr()
{
	DeleteCriticalSection(&cs);
	::WSACleanup();
}


// 支持完成端口
HRESULT CTransProTcpSvr::InitializeCompletePort(ICTEPTransferProtocolCallBack* piCallBack)
{
	HRESULT hr = E_FAIL;
	HANDLE hReturn = NULL;
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	GUID GuidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;		
	DWORD dwBytes;

	m_piCallBack = piCallBack;

	// 读取配置文件
	m_uPort;

	// 创建监听套节字，绑定到本地端口，进入监听模式
	m_sListen = ::WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN si;
	si.sin_family = AF_INET;
	si.sin_port = ::ntohs(m_uPort);
	si.sin_addr.S_un.S_addr = INADDR_ANY;
	if( ::bind(m_sListen, (sockaddr*)&si, sizeof(si)) == SOCKET_ERROR)
	{
		long dwErr = (long)WSAGetLastError();
		ASSERT(dwErr>0);
		hr = -1 * dwErr;
		goto End;
	}
	::listen(m_sListen, MAX_LISTEN_CONNECTION);
	// 加载扩展函数AcceptEx

	::WSAIoctl(m_sListen, 
		SIO_GET_EXTENSION_FUNCTION_POINTER, 
		&GuidAcceptEx, 
		sizeof(GuidAcceptEx),
		&m_lpfnAcceptEx, 
		sizeof(m_lpfnAcceptEx), 
		&dwBytes, 
		NULL, 
		NULL);

	// 加载扩展函数GetAcceptExSockaddrs
	::WSAIoctl(m_sListen,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidGetAcceptExSockaddrs,
		sizeof(GuidGetAcceptExSockaddrs),
		&m_lpfnGetAcceptExSockaddrs,
		sizeof(m_lpfnGetAcceptExSockaddrs),
		&dwBytes,
		NULL,
		NULL
		);

	if ( !m_lpfnAcceptEx || !m_lpfnGetAcceptExSockaddrs)
	{
		goto End;
	}

	// 将监听套节字关联到完成端口，注意，这里为它传递的CompletionKey为0
	hReturn = ::CreateIoCompletionPort((HANDLE)m_sListen
		, piCallBack->GetCompletePort(), (ULONG_PTR)0, 0);
	if ( !hReturn || hReturn != piCallBack->GetCompletePort())
	{
		CloseHandle(hReturn);
		goto End;
	}

	// 注册FD_ACCEPT事件。
	// 如果投递的AcceptEx I/O不够，线程会接收到FD_ACCEPT网络事件，说明应该投递更多的AcceptEx I/O
	if ( SOCKET_ERROR == WSAEventSelect(m_sListen, piCallBack->GetListenEvent(), FD_ACCEPT))
		hr = S_FALSE;	// 有警告,正确返回
	else
		hr = S_OK;

End:
	if ( FAILED(hr))
	{
		::closesocket(m_sListen);
		m_sListen = INVALID_SOCKET;
		if ( m_piCallBack)
		{
			m_piCallBack = NULL;
		}
	}

	return hr;
}

HRESULT CTransProTcpSvr::PostListen(bool bFirst)
{
	if ( INVALID_SOCKET == m_sListen || !m_piCallBack)
	{
		return E_FAIL;
	}

	int nCount = bFirst ? 10 : 1;
	::EnterCriticalSection(&cs);
	for ( int i = 0; i < nCount; i++)
	{
		ReadWritePacket *pPacket;
		pPacket = m_piCallBack->AllocatePacket(dynamic_cast<ICTEPTransferProtocolServer*>(this));
		m_piCallBack->InsertPendingAccept(pPacket);

		// 		// 设置I/O类型
		pPacket->opType = EmPacketOperationType::OP_Listen;

		// 投递此重叠I/O  
		DWORD dwBytes;
		pPacket->s = ::WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

		unsigned   long   sendbufsize = 0;
		int size = sizeof(sendbufsize);
		int iRet = getsockopt((SOCKET)pPacket->hFile, SOL_SOCKET, SO_RCVBUF, (char*)&sendbufsize, &size);
		if ( sendbufsize < 64*1024)
		{
			sendbufsize = 64*1024;
			iRet = setsockopt((SOCKET)pPacket->hFile, SOL_SOCKET, SO_RCVBUF, (char*)&sendbufsize, size);
		}
		sendbufsize = 0;
		iRet = getsockopt((SOCKET)pPacket->hFile, SOL_SOCKET, SO_SNDBUF, (char*)&sendbufsize, &size);
		if ( sendbufsize < 64*1024)
		{
			sendbufsize = 64*1024;
			iRet = setsockopt((SOCKET)pPacket->hFile, SOL_SOCKET, SO_SNDBUF, (char*)&sendbufsize, size);
		}

		BOOL b = m_lpfnAcceptEx(m_sListen, 
			pPacket->s,
			pPacket->buff.buff, 
			pPacket->buff.maxlength - ((sizeof(sockaddr_in) + 16) * 2),
			sizeof(sockaddr_in) + 16, 
			sizeof(sockaddr_in) + 16, 
			&dwBytes, 
			&pPacket->ol);
		if( !b && ::WSAGetLastError() != WSA_IO_PENDING)
		{
			::closesocket(pPacket->s);
			pPacket->s = INVALID_SOCKET;
			::LeaveCriticalSection(&cs);

			return E_FAIL;
		}
	}

	::LeaveCriticalSection(&cs);

	return S_OK;
}

SOCKET CTransProTcpSvr::GetListenSocket()
{
	return m_sListen;
}

// 返回指定连接已经建立的时间长度, TCP专用. 返回负数表示失败, -1表示不支持此功能
long CTransProTcpSvr::GetDuration(ReadWritePacket* pPacket)
{
	int nSeconds;
	int nLen = sizeof(nSeconds);

	// 取得连接建立的时间
	if ( pPacket->s != INVALID_SOCKET)
	{
		::getsockopt(pPacket->s, 
			SOL_SOCKET, SO_CONNECT_TIME, (char *)&nSeconds, &nLen);
	}
	else
	{
		return -2;
	}

	return nSeconds;
}
HRESULT CTransProTcpSvr::PostRecv(StTransferChannel* pTransChn, bool bFirst)
{
	if ( INVALID_SOCKET == m_sListen || !m_piCallBack)
	{
		return E_FAIL;
	}

	ReadWritePacket *pBuffer = m_piCallBack->AllocatePacket((ICTEPTransferProtocolServer*)this);
	if( pBuffer)
	{
		::EnterCriticalSection(&pTransChn->Lock);

		// 设置I/O类型
		pBuffer->opType = EmPacketOperationType::OP_Recv;

		// 投递此重叠I/O
		DWORD dwBytes;
		DWORD dwFlags = 0;
		WSABUF buf = {pBuffer->buff.maxlength, pBuffer->buff.buff};
		if(::WSARecv(pTransChn->s, &buf, 1, &dwBytes, &dwFlags, &pBuffer->ol, NULL) != NO_ERROR)
		{
			if(::WSAGetLastError() != WSA_IO_PENDING)
			{
				::LeaveCriticalSection( &pTransChn->Lock);
				m_piCallBack->FreePacket(pBuffer);

				return E_FAIL;
			}
		}

		// 增加套节字上的重叠I/O计数和读序列号计数
		::LeaveCriticalSection(&pTransChn->Lock);
		return S_OK;
	}

	return E_FAIL;
}

HRESULT CTransProTcpSvr::PostSend(StTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	// 设置I/O类型，增加套节字上的重叠I/O计数
	pPacket->opType = EmPacketOperationType::OP_Send;

	// 投递此重叠I/O
	DWORD dwBytes;
	DWORD dwFlags = 0;
	WSABUF buf = {pPacket->buff.size, pPacket->buff.buff};
	if( NO_ERROR != ::WSASend(pTransChn->s, &buf, 1, &dwBytes, dwFlags, &pPacket->ol, NULL))
	{
		long dwErr = (long)::WSAGetLastError();
		if( dwErr != WSA_IO_PENDING)
		{
			ASSERT(dwErr > 0);
			m_log.FmtErrorW(3, L"PostSend failed. ErrCode:%d", dwErr);
			return 0 - dwErr;
		}
	}

	return S_OK;
}

HRESULT CTransProTcpSvr::CompleteListen(StTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	// 取得客户地址
	int nLocalLen, nRmoteLen;
	LPSOCKADDR pLocalAddr, pRemoteAddr;

	pTransChn->type = TCP;

	m_lpfnGetAcceptExSockaddrs(
		pPacket->buff.buff,
		pPacket->buff.maxlength - ((sizeof(sockaddr_in) + 16) * 2),
		sizeof(sockaddr_in) + 16,
		sizeof(sockaddr_in) + 16,
		(SOCKADDR **)&pLocalAddr,
		&nLocalLen,
		(SOCKADDR **)&pRemoteAddr,
		&nRmoteLen);
	memcpy(&pTransChn->addrLocal, pLocalAddr, nLocalLen);
	memcpy(&pTransChn->addrRemote, pRemoteAddr, nRmoteLen);

	return S_OK;

}


// 不支持完成端口的
HRESULT CTransProTcpSvr::Initialize(ICTEPTransferProtocolCallBack* piCallBack)
{
	HRESULT hr = E_FAIL;
	HANDLE hReturn = NULL;
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	GUID GuidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
	int iResult;

	m_piCallBack = piCallBack;

	m_sListen = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( m_sListen == INVALID_SOCKET )
	{
		return E_FAIL;
	}

	// 填充sockaddr_in结构
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(m_uPort);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;

	iResult = ::bind(m_sListen, (LPSOCKADDR)&sin, sizeof(sin));
	if (iResult == SOCKET_ERROR)
		goto End;

	iResult = listen(m_sListen, MAX_LISTEN_CONNECTION);
	if (iResult == SOCKET_ERROR)
		goto End;


	return S_OK;
End:
	if ( FAILED(hr))
	{
		::closesocket(m_sListen);
		m_sListen = INVALID_SOCKET;

		m_piCallBack = NULL;
	}

	return hr;
}
HRESULT CTransProTcpSvr::Listen(StTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	sockaddr_in remoteAddr; 
	int nAddrLen = sizeof(remoteAddr);

	pTransChn->s = ::accept(m_sListen,(SOCKADDR*)&remoteAddr, &nAddrLen );
	if( pTransChn->s == INVALID_SOCKET)
	{
		return E_FAIL;
	}
	return S_OK;

}

//公共的
HRESULT CTransProTcpSvr::Recv(StTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	pPacket->buff.size = 0;
	int iRecv = ::recv(pTransChn->s, pPacket->buff.buff, pPacket->buff.maxlength, 0);
	DWORD dwErr = WSAGetLastError();
	if(  iRecv > 0 || dwErr == WSAETIMEDOUT)
	{
		pPacket->buff.size = iRecv;
		return S_OK;
	}

	return E_FAIL;
}


HRESULT CTransProTcpSvr::Send(StTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	HRESULT         hr = S_OK;

	int             bytesSent = 0;
	DWORD           &dwBufferSize = pPacket->buff.size;
	DWORD           dwAlreadySend = 0;

	while (dwAlreadySend < dwBufferSize)
	{
		bytesSent = ::send( pTransChn->s
			, pPacket->buff.buff+dwAlreadySend
			, dwBufferSize - dwAlreadySend
			, 0);
		if( bytesSent ==  SOCKET_ERROR || bytesSent <= 0 )
		{
			hr = E_FAIL;
			break;
		}
		dwAlreadySend += bytesSent;
	}

	return hr;
}

HRESULT CTransProTcpSvr::Disconnect(StTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	if ( pPacket && pTransChn)
	{
		ASSERT(pPacket->hFile == pPacket->hFile);
	}

	if ( pPacket)
	{
		ASSERT(!pTransChn);
		ASSERT( pPacket->opType == EmPacketOperationType::OP_Listen);
		SOCKET s= (SOCKET)pPacket->hFile;
		pPacket->hFile = INVALID_HANDLE_VALUE;
		::closesocket(s);
	}

	if ( pTransChn)
	{
		ASSERT(!pPacket);
		SOCKET s = (SOCKET)pTransChn->hFile;
		pTransChn->hFile = INVALID_HANDLE_VALUE;
		::closesocket(s);
	}

	if ( !pPacket && !pTransChn)
	{
		ASSERT(0);
		return E_FAIL;
	}

	return S_OK;
}

void CTransProTcpSvr::Final()
{
	if ( m_sListen != INVALID_SOCKET)
	{
		::closesocket(m_sListen);
		m_sListen = INVALID_SOCKET;
	}

	m_piCallBack = NULL;
}












