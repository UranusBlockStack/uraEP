// CTEPTStestTcp.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "CTEPTCTcp.h"

CTransProTcpClt gOne;

#define DEFAULT_PORT 4567


ICTEPTransferProtocolClient* WINAPI CTEPGetInterfaceTransClient()
{
	return dynamic_cast<ICTEPTransferProtocolClient*>(&gOne);
}

CTransProTcpClt::CTransProTcpClt()
	: m_piCallBack(nullptr), m_log("TC_TCP"), m_uPort(DEFAULT_PORT)
	, m_pTransChn(nullptr)
{
	// 初始化WS2_32.dll
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(2, 2);
	::WSAStartup(sockVersion, &wsaData);
}
CTransProTcpClt::~CTransProTcpClt()
{
	::WSACleanup();
}

HRESULT CTransProTcpClt::Initialize(ICTEPTransferProtocolClientCallBack* pI)
{
	ASSERT(!m_piCallBack);
	m_piCallBack = pI;
	return S_OK;
}

HRESULT CTransProTcpClt::Connect(StTransferChannel* pTransChn, ReadWritePacket* pPacket /*= 0*/)
{
	HRESULT hr = E_FAIL;
	DWORD dwErr = 0;
	int iRet;

	if ( m_pTransChn || !m_piCallBack)
	{
		ASSERT(0);
		return E_FAIL;
	}

	pTransChn->s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( pTransChn->s == INVALID_SOCKET)
		goto End;

	iRet = ::connect(pTransChn->s
		 , (sockaddr*)&pTransChn->addrRemote, sizeof(pTransChn->addrRemote));
	if ( iRet != 0)
	{
		dwErr = WSAGetLastError();
		goto End;
	}

	m_pTransChn = pTransChn;
	m_piCallBack->Connected(pTransChn);

	if ( pPacket)
	{
		pPacket->s = pTransChn->s;
		hr = Send(pTransChn, pPacket);
	}
	else
		hr = S_OK;

End:
	if ( FAILED(hr))
	{
		m_pTransChn = nullptr;
		closesocket(pTransChn->s);
		pTransChn->s = INVALID_SOCKET;
	}

	return hr;
}

//公共的
HRESULT CTransProTcpClt::Recv(StTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	pPacket->buff.size = 0;
	ASSERT(pTransChn == m_pTransChn || m_pTransChn == nullptr);
	if ( m_pTransChn == nullptr)
		return E_FAIL;

	lckRecv.Lock();
	int iRecv = ::recv(pTransChn->s, pPacket->buff.buff, pPacket->buff.maxlength, 0);
	lckRecv.Unlock();

	DWORD dwErr = WSAGetLastError();
	if(  iRecv > 0 || dwErr == WSAETIMEDOUT)
	{
		pPacket->buff.size = iRecv;
		m_piCallBack->Recv(pTransChn, pPacket);
		return S_OK;
	}

	return E_FAIL;
}


HRESULT CTransProTcpClt::Send(StTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	ASSERT(pTransChn == m_pTransChn || m_pTransChn == nullptr);
	if ( m_pTransChn == nullptr)
		return E_FAIL;

	HRESULT         hr = S_OK;

	int             bytesSent = 0;
	DWORD			dwErr = 0;
	DWORD           &dwBufferSize = pPacket->buff.size;
	DWORD           dwAlreadySend = 0;

	lckSend.Lock();
	while (dwAlreadySend < dwBufferSize)
	{
		bytesSent = ::send( pTransChn->s
			, pPacket->buff.buff+dwAlreadySend
			, dwBufferSize - dwAlreadySend
			, 0);
		if( bytesSent ==  SOCKET_ERROR || bytesSent <= 0 )
		{
			dwErr = WSAGetLastError();
			hr = E_FAIL;
			break;
		}
		dwAlreadySend += bytesSent;
	}
	lckSend.Unlock();

	ASSERT(dwAlreadySend == dwBufferSize || FAILED(hr));
	return hr;
}

HRESULT CTransProTcpClt::Disconnect(StTransferChannel* pTransChn)
{
	ASSERT(pTransChn == m_pTransChn || m_pTransChn == nullptr);
	if ( pTransChn && pTransChn->s != INVALID_SOCKET)
	{
		SOCKET s = pTransChn->s;
		pTransChn->s = INVALID_SOCKET;
		::closesocket(s);
	}

	if ( m_pTransChn)
	{
		m_pTransChn = nullptr;
	}

	m_piCallBack->Disconnected(pTransChn, 0);

	return S_OK;
}

void CTransProTcpClt::Final()
{
	m_piCallBack = NULL;
}












