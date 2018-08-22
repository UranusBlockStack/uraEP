#include "stdafx.h"

#include "CommonInclude/Tools/AdapterInformation.h"

#include "VirtualChannelManager.h"
#include "IcaTransferClient.h"

CICAVirtualChannelManager::CICAVirtualChannelManager(PVD pVd)
	: m_ChannelMask(0), m_log("TC_ICA")
{
	ASSERT(pVd);
	m_pVd = pVd;
}

HANDLE CICAVirtualChannelManager::InitChannel(const char* pChannelName)
{
	USHORT usChannelNum = 0;
	ULONG  ulChannelMask = 0;
	VDWRITEHOOK vdwh = {0};
	int rc = CLIENT_STATUS_SUCCESS;
	char szMsg[MAX_PATH] = {0};

	// 注册通道
	rc = RegisterVirtualChannel(m_pVd, (char*)pChannelName, &usChannelNum, &ulChannelMask);
	ASSERT(rc == CLIENT_STATUS_SUCCESS);
	if ( rc != CLIENT_STATUS_SUCCESS )
		return INVALID_HANDLE_VALUE;
	
	//获取读写回调函数
	rc = RegisterWriteHook(m_pVd, usChannelNum, &vdwh);
	ASSERT(rc == CLIENT_STATUS_SUCCESS);
	if ( rc != CLIENT_STATUS_SUCCESS )
		return INVALID_HANDLE_VALUE;

	// Grab points to functions to use send data to the hosts;
	strcpy_s(szChannelName, pChannelName);
	bChannelOpened = TRUE;
	pQueueVirtualWrite = vdwh.pQueueVirtualWriteProc;

	ASSERT(vdwh.pQueueVirtualWriteProc && pQueueVirtualWrite);
	usVCNum = usChannelNum;
	ASSERT(listPacketsSent.IsEmpty());

	ASSERT(vdwh.pWdData);
	pWd                 = vdwh.pWdData;
	pOutBufReserveProc  = vdwh.pOutBufReserveProc;
	pOutBufAppendProc   = vdwh.pOutBufAppendProc;
	pOutBufWriteProc    = vdwh.pOutBufWriteProc;
	pAppendVdHeaderProc = vdwh.pAppendVdHeaderProc;

	m_maxDataSize = vdwh.MaximumWriteSize - 10;
	m_ChannelMask = ulChannelMask;

	m_log.FmtMessageW(2, L"InitChannel: channelName = %S,Max_Data_Size is :%d"
		,pChannelName, m_maxDataSize);

	ASSERT(pQueueVirtualWrite);
	ASSERT(pWd);

	return CLIENT_STATUS_SUCCESS;
}

BOOL CICAVirtualChannelManager::WriteDataToChannel(ReadWritePacket* pPacket)
{
	ASSERT(pPacket);
	if ( !pPacket)
		return TRUE;

	listPacketsSent.Push(pPacket, TRUE);
	m_log.FmtMessage(1, L"WriteDataToChannel - Push to list. Size:%d", pPacket->buff.size);
// 	int ret = SendDataDirect(pPacket);
// 	if (ret < 0)
// 	{
// 		m_log.FmtError(3, L"WriteDataToChannel - SendDataDirect Failed. size:%d", pPacket->buff.size);
// 		return FALSE;
// 	}
	return TRUE;
}

int CICAVirtualChannelManager::SendDataDirect(ReadWritePacket* pPacket)
{
	ASSERT(bChannelOpened && pPacket);
	DWORD &dwGetSize = pPacket->buff.size;

	m_log.FmtMessageW(1, L"SendDataDirect: size[%d], m_usVCNum[%d], m_pWd[0x%08x] pQueueVirtualWrite:[0x%08x]"
		, dwGetSize, usVCNum, pWd, pQueueVirtualWrite);

	int ret = CLIENT_STATUS_SUCCESS;
	ASSERT(pQueueVirtualWrite);
	if ( pQueueVirtualWrite)
	{
		MEMORY_SECTION memsec = {0};
		memsec.pSection = (LPBYTE)pPacket->buff.buff;
		memsec.length = dwGetSize;
		ASSERT(pWd);
		ASSERT(usVCNum);
		USHORT temp = 25;
		ret = pQueueVirtualWrite(pWd, usVCNum, &memsec, 1, FLUSH_IMMEDIATELY);
		m_log.FmtMessage(2, L"SendDataDirect - return:%d Size:%d", ret, dwGetSize);
		if ( ret != CLIENT_STATUS_SUCCESS)
		{
			m_log.FmtErrorW(3, L"SendDataDirect - m_pQueueVirtualWrite. ret:%d"
				, ret);
		}
	}

	return ret;
}
void CICAVirtualChannelManager::DriveClose()
{
	m_log.MessageW(1, L"DriveClose.");
	ASSERT(bChannelOpened);

	if (bChannelOpened)
	{
		bChannelOpened = FALSE;
	}

	m_ChannelMask = 0;
	m_pVd = NULL;
}
int CICAVirtualChannelManager::IcaPollData(PVD pVd, PDLLPOLL pVdPoll, PUINT16 puiSize)
{
	static BOOL bFirst = TRUE;
	if ( bFirst)
	{
		bFirst = FALSE;
		m_log.print(L"IcaPollData in.");
	}

	int rc = CLIENT_STATUS_NO_DATA;
	ReadWritePacket* pPacket = listPacketsSent.Pop();
	if ( !pPacket)
		goto Exit;

	/*
	*  Write is postponed until this poll loop because the outbufreserve can
	*  fail, forcing us to wait until a buffer is freed - the only way to wait
	*  is to pick things up during the poll loop.
	*
	*  Reserve output buffer space; fudge the length for the ICA virtual
	*  header overhead.
	*/
	m_log.FmtMessage(1, L"IcaPollData. DataSize:%d, DataSizeMax:%d", pPacket->buff.size, m_maxDataSize);

	rc = SendDataDirect(pPacket);
	if ( rc == CLIENT_STATUS_SUCCESS)
		return rc;

	ASSERT(pPacket->buff.size < m_maxDataSize);
	rc = pOutBufReserveProc( pWd, (USHORT)(pPacket->buff.size + 4));
	if ( rc != CLIENT_STATUS_SUCCESS)
	{
		m_log.FmtError(2, L"IcaPollData - pOutBufReserveProc failed. rc:%d", rc);
		rc =  CLIENT_STATUS_ERROR_RETRY;
		goto Exit;
	}

	/*
	*  Append ICA Virtual write header
	*/
	rc = pAppendVdHeaderProc( pWd, usVCNum, (USHORT)pPacket->buff.size);
	if ( rc != CLIENT_STATUS_SUCCESS)
	{
		m_log.FmtError(2, L"IcaPollData - pAppendVdHeaderProc failed. rc:%d", rc);
		goto Exit;
	}

	/*
	*  Append the ping packet
	*/
	rc = pOutBufAppendProc( pWd, (LPBYTE)pPacket->buff.buff, (USHORT)pPacket->buff.size);
	if ( rc != CLIENT_STATUS_SUCCESS )
	{
		m_log.FmtError(2, L"IcaPollData - pOutBufAppendProc failed. rc:%d", rc);
		goto Exit;
	}

	/*
	*  Write the buffer
	*/
	rc = pOutBufWriteProc(pWd);
	if ( rc != CLIENT_STATUS_SUCCESS )
	{
		m_log.FmtError(2, L"IcaPollData - pOutBufWriteProc failed. rc:%d", rc);
		goto Exit;
	}

	m_log.Message(2, L"IcaPollData - OutBufWrite Success.");

	rc = CLIENT_STATUS_SUCCESS;

Exit:
	return rc;

}


// 
// class CIcaTransferClient : public CICAVirtualChannelManager
// 
HRESULT CIcaTransferClient::Initialize(ICTEPTransferProtocolClientCallBack* pI)
{
	m_log.FmtMessage(1, L"Initialize. pI:[0x%08x] piCallBack:[0x%08x]", pI, m_piCallBack);
	ASSERT(!m_piCallBack || m_piCallBack == pI);
	ASSERT(pI && dynamic_cast<ICTEPTransferProtocolClient*>(this));
	m_piCallBack = pI;
	return S_OK;
}

HRESULT CIcaTransferClient::Connect(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	ASSERT(m_pTransChnn == nullptr);
	AdapterInfomation::GetTCPLocalProcessInfo(&pTransChn->addrLocal, &pTransChn->addrRemote, 1);
	pTransChn->hFile = m_pVd;
	m_pTransChnn = pTransChn;

	if ( pPacket)
		return Send(pTransChn, pPacket);

	return S_OK;
}
HRESULT CIcaTransferClient::Disconnect(CTransferChannel* pTransChn)
{
	if ( pTransChn && pTransChn->hFile != INVALID_HANDLE_VALUE)
	{
		pTransChn->hFile = INVALID_HANDLE_VALUE;
		ASSERT(pTransChn == m_pTransChnn);
		m_pTransChnn = nullptr;
		m_piCallBack->Recv(pTransChn, 0);
	}
	return S_OK;
}
HRESULT CIcaTransferClient::Send(CTransferChannel* pTransChn,ReadWritePacket* pPacket)
{
	m_log.FmtMessageW(1, L"CTEP_ICA Send: size(%d)", pPacket->buff.size);
	int ret = WriteDataToChannel(pPacket);
	if( ret != 1)
	{
		m_log.FmtErrorW(1, L"CTEP_ICA Send Failed. size(%d) Ret:%d", pPacket->buff.size, ret);
		return E_FAIL;
	}

	return ERROR_IO_PENDING;
}

void CIcaTransferClient::RecvData(char* pbuff,DWORD size)
{
	ASSERT(m_piCallBack);
	static ReadWritePacket* pPacket = m_piCallBack->AllocatePacket(this);
	ASSERT(pPacket);
	pPacket->opType = OP_IocpRecv;
	pPacket->buff.buff = pbuff;
	pPacket->buff.size = size;

	m_log.FmtMessageW(1, L"CTEP_ICA Recv: size(%d)", size);
	m_piCallBack->Recv(m_pTransChnn, pPacket);
}













