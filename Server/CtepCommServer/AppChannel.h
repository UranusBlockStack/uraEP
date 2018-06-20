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

	SerialNumColl<CUserDataEx*, USHORT> m_smapUser;
	SerialNumColl<CAppChannelEx*, USHORT> m_smapAppChn;

	in_addr m_LocalIPv4[10];//记录了本地机器的IP地址
	DWORD	m_nIPv4Count;

public:
	CCtepCommunicationServer():m_log("Svr")
	{
		m_nIPv4Count = sizeof(m_LocalIPv4)/sizeof(m_LocalIPv4[0]);
		AdapterInfomation::RetrieveAdapterIPv4(m_LocalIPv4, m_nIPv4Count);
	}

public:
//interface ICTEPAppProtocolCallBack
	virtual CAppChannel* LockChannel(USHORT AppChannelId) override
	{
		CAppChannelEx* pFind = m_smapAppChn.Find(AppChannelId);
		if ( pFind && pFind->pUser)
		{
			CUserDataEx* pUserEx = (CUserDataEx*)pFind->pUser;
			pUserEx->Lock();
			if ( pUserEx->bClosing)
			{
				pUserEx->Unlock();
				pFind = nullptr;
			}
		}
		return pFind;
	}
	virtual void UnlockChannel(CAppChannel* pAppChannel) override
	{
		if ( pAppChannel && pAppChannel->pUser)
		{
			((CUserDataEx*)pAppChannel->pUser)->Unlock();
		}
	}

	virtual ReadWritePacket*	MallocPacket(CAppChannel *pAppChn, ULONG size = 0) override// 创建发送包内存
	{
		return AllocateBuffer(pAppChn->pTransChannel, EmPacketOperationType::OP_Send, size);
	}
	virtual void FreePacket(ReadWritePacket *p) override
	{
		ReleaseBuffer(p);
	}

	virtual HRESULT WritePacket(CAppChannel *pAppChn, char* buff, ULONG size) override;
	virtual HRESULT WritePacket(CAppChannel *pAppChn, ReadWritePacket *pPacket) override
	{
		BOOL bRet = SendPacket((StTransferChannelEx*)pAppChn->pTransChannel, pPacket, pAppChn->Level);
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

	CAppChannel* allocateAppChannel(CUserDataEx *user, ICTEPAppProtocol* piAppProt
		, CAppChannelEx *pStaAppChn = nullptr, EmPacketLevel level = Middle, USHORT option = 0);
	void releaseAppChannel(CAppChannel* pAppChn);

	CUserDataEx* allocateUser(StTransferChannelEx* pTransChnMain);
	void releaseUser(CUserDataEx* pUserEx);

private:// Event Impl
	virtual BOOL OnStart() override;
	virtual void OnShutdown() override;

	virtual void OnConnectionEstablished(StTransferChannelEx *pContext, ReadWritePacket *pBuffer) override;
	virtual void OnReadCompleted(StTransferChannelEx *pContext, ReadWritePacket *pBuffer) override;
	virtual void OnConnectionClosing(StTransferChannelEx *pContext, ReadWritePacket *pBuffer) override;
#ifdef _DEBUG
	virtual BOOL OnConnectionError(StTransferChannelEx *pContext, ReadWritePacket *pBuffer, int nError) override
	{
		UNREFERENCED_PARAMETER(pBuffer);
		m_log.wprint(L" 一个连接发生错误[0x%08x]： %d[0x%08x] \n ", pContext, nError, nError);
		return FALSE;// 返回FALSE表示不处理
	}
#endif // _DEBUG
};
































