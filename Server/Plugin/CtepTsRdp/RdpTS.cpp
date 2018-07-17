// CTEPTStestTcp.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "RdpTS.h"
#include <list>
using namespace std;

CTransProRdpSvr gOne;

ICTEPTransferProtocolCallBack* m_piCallBack;

ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServer()
{
	return dynamic_cast<ICTEPTransferProtocolServer*>(&gOne);
}

HRESULT CTransProRdpSvr::InitializeCompletePort(ICTEPTransferProtocolCallBack* piCallBack)
{
	ASSERT(piCallBack);
	if( !CtepTsRdpMainThreadInit())
		return E_FAIL;

	m_piCallBack = piCallBack;
	return S_OK;
}

HRESULT CTransProRdpSvr::PostListen(bool bFirst)
{
	return S_OK;
}

HRESULT CTransProRdpSvr::CompleteListen(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	if( pPacket->hFile == INVALID_HANDLE_VALUE)
		return E_FAIL;

	StListenData *pListenData = (StListenData*)pPacket->buff.buff;
	ASSERT(pListenData->dwMagic == 0x12344321 && pPacket->buff.size == 0);
	if ( pListenData->dwMagic != 0x12344321)
		return E_FAIL;

	CTransferChannel* pTransChnFind = nullptr;
	LOCK(&lckMap);
	auto pair = g_mapSessionData.Lookup(pListenData->dwSessionId);
	if ( pair)
	{
		pTransChnFind = pair->m_value;
		if ( pTransChnFind == (CTransferChannel*)-1)
		{
			g_mapSessionData[pListenData->dwSessionId] = pTransChn;
			pTransChn->hFile = pListenData->hRdpChannel;
			pTransChn->type = SyncMain;
			pTransChn->dwSessionId = pListenData->dwSessionId;

			return S_OK;
		}
	}

	ASSERT(0);
	return E_INVALIDARG;
}


HRESULT CTransProRdpSvr::PostRecv(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	HANDLE hFile = pTransChn->hFile;
	ASSERT(pTransChn && pPacket);
	ASSERT(pPacket->hFile == pTransChn->hFile || pTransChn->bClosing || pTransChn->hFile == INVALID_HANDLE_VALUE);
	ASSERT(pPacket->opType == EmPacketOperationType::OP_IocpRecv);

	if ( hFile == INVALID_HANDLE_VALUE)
	{
		pPacket->ol.Internal = ERROR_HANDLES_CLOSED;
		return E_NOINTERFACE;
	}

	if ( !m_piCallBack){ASSERT(m_piCallBack);	return E_FAIL;}
	if( !pPacket){ASSERT(pPacket);	return E_FAIL;}

	pPacket->opType = EmPacketOperationType::OP_IocpRecv;

	BOOL bRet = WTSVirtualChannelRead(hFile, 0, pPacket->buff.buff, pPacket->buff.maxlength, &pPacket->ol.InternalHigh);
	if ( !bRet)
	{
		int dwErr = GetLastError();
		if ( dwErr == 0)
			dwErr = -1;
		pPacket->ol.Internal = dwErr;
		if ( dwErr>0)
		{
			return -1*dwErr;
		}

		return E_FAIL;
	}

	return S_OK;
}

HRESULT CTransProRdpSvr::PostSend(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	HANDLE hFile = pTransChn->hFile;
	ASSERT(hFile);
	ASSERT(pTransChn && pPacket);
	ASSERT(pPacket->hFile == pTransChn->hFile);
	ASSERT(pPacket->opType == EmPacketOperationType::OP_IocpSend);

	if ( pTransChn->bClosing || hFile == INVALID_HANDLE_VALUE)
	{
		ASSERT(pTransChn->bClosing && hFile == INVALID_HANDLE_VALUE);
		return E_NOINTERFACE;
	}
	
	pPacket->opType = EmPacketOperationType::OP_IocpSend;
	DWORD dwSent = 0;
	BOOL bRet = WTSVirtualChannelWrite(hFile, pPacket->buff.buff, pPacket->buff.size, &dwSent);
	if ( bRet && dwSent == pPacket->buff.size)
	{
#ifdef _DEBUG
		memset(pPacket->buff.buff, 0xAB, pPacket->buff.size);
#endif // _DEBUG
		return S_OK;
	}
	DWORD dwErr = GetLastError();
	ASSERT(ERROR_GEN_FAILURE == dwErr);
	return E_FAIL;
}

HRESULT CTransProRdpSvr::Disconnect(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	ASSERT(pTransChn);
	if ( pPacket && pTransChn)
	{
		ASSERT(pTransChn->hFile == pPacket->hFile);
	}

	if ( pTransChn)
	{
		HANDLE hFile = pTransChn->hFile;
		if ( hFile != INVALID_HANDLE_VALUE)
		{
			ASSERT(!pPacket && hFile);
			pTransChn->hFile = INVALID_HANDLE_VALUE;
			CRdpChannelServer::RdpChannelClose((HANDLE)hFile);
		}
	}

	if ( !pPacket && !pTransChn)
	{
		ASSERT(0);
		return E_FAIL;
	}

	return S_OK;
}

void CTransProRdpSvr::Final()
{
	CtepTsRdpMainThreadClose();
	m_piCallBack = NULL;
}





SOCKET CTransProRdpSvr::GetListenSocket()
{
	return INVALID_SOCKET;
}
long CTransProRdpSvr::GetDuration(ReadWritePacket* pPacket)
{
	return -1;
}
