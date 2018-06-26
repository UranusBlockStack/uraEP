#pragma once

// CIOCPServer类的测试程序

#include "Iocp/iocp.h"
#include "CommonInclude/Tools/AdapterInformation.h"

#include "CommonInclude/Tools/SerialNumColloction.h"
#pragma comment(lib, "Rpcrt4")//UuidCreate()


class CCtepCommunicationServer
	: public CIOCPServer , public ICTEPAppProtocolCallBack
{
	Log4CppLib m_log;

	FastQueue<CAppChannelEx> m_queFreeAppChn;
	SerialNumColl<CUserDataEx*, USHORT> m_smapUser;
	SerialNumColl<CAppChannelEx*, USHORT> m_smapAppChn;

	in_addr m_LocalIPv4[10];//记录了本地机器的IP地址
	DWORD	m_nIPv4Count;

public:
	CCtepCommunicationServer():m_log("Svr")
	{
		CAppChannelEx* pAppChn = new CAppChannelEx();
		pAppChn->Recycling();
		m_queFreeAppChn.Initialize(pAppChn, TRUE, TRUE, 150);

		m_nIPv4Count = sizeof(m_LocalIPv4)/sizeof(m_LocalIPv4[0]);
		AdapterInfomation::RetrieveAdapterIPv4(m_LocalIPv4, m_nIPv4Count);
	}

public:
//interface ICTEPAppProtocolCallBack
	virtual CAppChannel* LockChannel(USHORT AppChannelId) override
	{
		CAppChannelEx* pAppChannel = m_smapAppChn.Find(AppChannelId);
		if ( !pAppChannel)
			return nullptr;

		CUserDataEx* pUserEx = (CUserDataEx*)pAppChannel->pUser;
		if ( !pUserEx || pUserEx->bClosing)
			return nullptr;

		pAppChannel->Lock();
		return pAppChannel;
	}

	virtual void UnlockChannel(CAppChannel* pAppChannel) override
	{
		ASSERT(pAppChannel);
		if ( pAppChannel)
		{
			((CAppChannelEx*)pAppChannel)->Unlock();
		}
	}

	virtual ReadWritePacket*	MallocSendPacket(CAppChannel *pAppChn, USHORT size) override// 创建发送包内存
	{
		ASSERT(size <= CTEP_DEFAULT_BUFFER_DATA_SIZE);
		if ( size > CTEP_DEFAULT_BUFFER_DATA_SIZE)
			return nullptr;

		CUserData* pUser = ((CAppChannelEx*)pAppChn)->pUser;
		ReadWritePacket* pBuffer = AllocateBuffer(
			((CAppChannelEx*)pAppChn)->pTransChannel, EmPacketOperationType::OP_IocpSend, 0);
		if ( pBuffer)
		{
			CTEPPacket_Message* pMsg = (CTEPPacket_Message*)pBuffer->buff.buff;
			pBuffer->buff.size = Create_CTEPPacket_Message(pMsg, pUser->UserId, pAppChn->AppChannelId, 0, size);

			pBuffer->pointer = pMsg->GetBuffer();
		}

		return pBuffer;
	}
	virtual void FreePacket(ReadWritePacket *p) override
	{
		ReleaseBuffer(p);
	}

	virtual HRESULT WritePacket(CAppChannel *pAppChn, char* buff, ULONG size) override;
	virtual HRESULT WritePacket(CAppChannel *pAppChn, ReadWritePacket *pPacket) override
	{
		BOOL bRet = SendPacket(((CAppChannelEx*)pAppChn)->pTransChannel, pPacket, pAppChn->Level);
		return bRet ? S_OK : E_FAIL;
	}

	virtual CAppChannel* CreateDynamicChannel(CAppChannel* pStaticChannel
		 , EmPacketLevel level = Middle, USHORT option = NULL) override;// CAppChannel::uPacketOption option
	virtual void	CloseDynamicChannel(CAppChannel* pDynamicChannel) override;

private://internal function
// 	CAppChannelEx* CreateStaticChannel(CUserDataEx *user, ICTEPAppProtocol* piAppProt
// 		, EmPacketLevel level = Middle, USHORT option = 0);// CAppChannel::uPacketOption option
	void	CloseStaticChannel(CAppChannel* pStaticChannel);

	inline void closeAppChannel(CAppChannel* pAppChannel);			// close static channel.

	CAppChannelEx* allocateAppChannel(CUserDataEx *user, ICTEPAppProtocol* piAppProt
		, CAppChannelEx *pStaAppChn = nullptr, EmPacketLevel level = Middle, USHORT option = 0);
	void releaseAppChannel(CAppChannelEx* pAppChnEx);

	CUserDataEx* allocateUser(CTransferChannelEx* pTransChnMain);
	void releaseUser(CUserDataEx* pUserEx);

	inline ReadWritePacket*	MallocPacket(CAppChannel *pAppChn, ULONG size)// 创建发送包内存
	{
		return AllocateBuffer(((CAppChannelEx*)pAppChn)->pTransChannel, EmPacketOperationType::OP_IocpSend, size);
	}

private:// Event Impl
	virtual BOOL OnStart() override;
	virtual void OnShutdown() override;

	virtual void OnConnectionEstablished(CTransferChannelEx *pContext, ReadWritePacket *pBuffer) override;
	virtual void OnReadCompleted(CTransferChannelEx *pContext, ReadWritePacket *pBuffer) override;
	virtual void OnConnectionClosing(CTransferChannelEx *pContext, ReadWritePacket *pBuffer) override;
#ifdef _DEBUG
	virtual BOOL OnConnectionError(CTransferChannelEx *pContext, ReadWritePacket *pBuffer, int nError) override
	{
		UNREFERENCED_PARAMETER(pBuffer);
		m_log.FmtWarning(5, L" 一个连接发生错误[0x%08x]-Socket(%d) ErrCode:%d[0x%08x] \n ", pContext, (DWORD)pContext->hFile, nError, nError);
		return FALSE;// 返回FALSE表示不处理
	}
#endif // _DEBUG
};
































