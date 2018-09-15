#pragma once

// CIOCPServer类的测试程序

#include "Iocp/iocp.h"
#include "CommonInclude/Tools/AdapterInformation.h"

#include "CommonInclude/Tools/SerialNumColloction.h"
#include "CommonInclude/Tools/ProcessThread.h"

#pragma comment(lib, "Rpcrt4")//UuidCreate()

class CSessionMessageCallBackList : public CallBackList
{
public:
	CSessionMessageCallBackList():CallBackList(StCallEvent::SessionEvent){}

	void evtSessionMessage(WPARAM wParam, LPARAM lParam)
	{
		LOCK(&lck);
		for ( DWORD i=0; i < this->lstCount; i++)
		{
			if ( lstCall[i].fnSessionEvent)
				lstCall[i].fnSessionEvent(lstCall[i].pParam, wParam, lParam);
		}
	}
};

class CCtepCommunicationServer
	: public CIOCPServer , public ICTEPAppProtocolCallBackEx
{
	Log4CppLib m_log;

	FastQueue<CAppChlEx>			  m_queFreeAppChn;
	FastQueue<CUserDataEx>			  m_queFreeUserData;
	
	SerialNumColl<CUserDataEx*,   USHORT> m_smapUser;
	SerialNumColl<CAppChlEx*, USHORT> m_smapAppChn;


	in_addr m_LocalIPv4[10];//记录了本地机器的IP地址
	DWORD	m_nIPv4Count;

	CWorkerThread<CCtepCommunicationServer>	m_ThreadCallBack;
	DWORD TrdCallBackEventWorker(LPVOID, HANDLE, CWorkerThread<CCtepCommunicationServer>*);
public:
	CSessionMessageCallBackList m_CallBackList_SessionEvent;

public:
	CCtepCommunicationServer():m_log("Svr")
	{
		m_ThreadCallBack.Initialize(this, &CCtepCommunicationServer::TrdCallBackEventWorker);

		CAppChlEx* pAppChn = new CAppChlEx();
		pAppChn->Recycling();
		m_queFreeAppChn.Initialize(pAppChn, TRUE, TRUE, 150);

		CUserDataEx* pUserEx = new CUserDataEx();
		pUserEx->Recycling();
		m_queFreeUserData.Initialize(pUserEx, TRUE, TRUE, 20);

		m_nIPv4Count = sizeof(m_LocalIPv4)/sizeof(m_LocalIPv4[0]);
		AdapterInfomation::RetrieveAdapterIPv4(m_LocalIPv4, m_nIPv4Count);
	}

public:
	virtual ULONG GetCurrentConnection()
	{
		//LOCK(&m_smapUser);
		return (ULONG)m_smapUser.GetCount();
	}

public:
//interface CTEP 2.0
	_VIRT_D GetInterfaceVer() override {return 0x0200;}
	_VIRT(CAppChl*) CreateDynamicChannelLock(USHORT AppChannelId
		, EmPacketLevel level = Middle, USHORT option = NULL) override
	{
		CAppChl* pResult = nullptr;
		CAppChl* pStaticChannel = LockChannel(AppChannelId, nullptr);
		if ( pStaticChannel)
		{
			pResult = createDynamicChannel(pStaticChannel, level, option);
			UnlockChannel(pStaticChannel);
		}
		return pResult;
	}
	_VIRT(CAppChl*) CreateDynamicChannelLock(CAppChl* pStaticChannel
		, EmPacketLevel level = Middle, USHORT option = NULL) override
	{
		if ( pStaticChannel)
			return createDynamicChannel(pStaticChannel, level, option);
		return nullptr;
	}

	_VIRT_H RegisterCallBackEvent(StCallEvent Calls[], DWORD dwCallCount) override
	{
		HRESULT hr = S_OK;
		ASSERT(Calls && dwCallCount > 0);
		for (DWORD i = 0; i < dwCallCount; i++)
		{
			if ( Calls[i].type == StCallEvent::SessionEvent)
			{
				m_CallBackList_SessionEvent.Insert(Calls[i]);
			}
			else
			{
				ASSERT(NULL);
				Calls[i].fnBase = nullptr;
				hr = S_FALSE;
			}
		}

		return hr;
	}
	_VIRT_H UnregisterCallBackEvent(StCallEvent Calls[], DWORD dwCallCount) override
	{
		HRESULT hr = S_OK;
		ASSERT(Calls && dwCallCount > 0);
		for (DWORD i = 0; i < dwCallCount; i++)
		{
			if ( Calls[i].type == StCallEvent::SessionEvent)
			{
				HRESULT hrRemove = m_CallBackList_SessionEvent.Remove(Calls[i].fnBase);
				if ( SUCCEEDED(hrRemove))
				{
					Calls[i].fnBase = nullptr;
				}
				else
				{
					hr = hrRemove;
				}
			}
			else
			{
				hr = S_FALSE;
			}
		}

		return hr;
	}

//interface ICTEPAppProtocolCallBack
	virtual CAppChl* LockChannel(USHORT AppChannelId, CUserData* pUser/* = nullptr*/) override
	{
		CAppChlEx* pAppChannel;
		pAppChannel = m_smapAppChn.Find(AppChannelId);
		if ( !pAppChannel)
			return nullptr;

		CUserDataEx* pUserEx = (CUserDataEx*)pAppChannel->pUser;
		ASSERT(pAppChannel->pUser == pUser || pUser == nullptr);
		ASSERT(pAppChannel->pStaticAppChannel == pUserEx->FindApp(pAppChannel->pStaticAppChannel->AppChannelId));
		if ( !pUserEx || pUserEx->Status == User_Invalid)
			return nullptr;

		pAppChannel->Lock();
		return pAppChannel;
	}

	_VIRT_V UnlockChannel(CAppChl* pAppChannel) override
	{
		ASSERT(pAppChannel);
		if ( pAppChannel)
		{
			((CAppChlEx*)pAppChannel)->Unlock();
		}
	}

	_VIRT(RWPacket*)	MallocSendPacket(CAppChl *pAppChn, USHORT size) override// 创建发送包内存
	{
		ASSERT(size <= CTEP_DEFAULT_BUFFER_DATA_SIZE);
		if ( size > CTEP_DEFAULT_BUFFER_DATA_SIZE)
			return nullptr;

		CUserData* pUser = ((CAppChlEx*)pAppChn)->pUser;
		ReadWritePacket* pBuffer = AllocateBuffer(
			((CAppChlEx*)pAppChn)->pTransChannel, EmPacketOperationType::OP_IocpSend, 0);
		if ( pBuffer)
		{
			CTEPPacket_Message* pMsg = (CTEPPacket_Message*)pBuffer->buff.buff;
			pBuffer->buff.size = Create_CTEPPacket_Message(pMsg, pUser->UserId, pAppChn->AppChannelId, 0, size);

			pBuffer->pointer = pMsg->GetBuffer();
		}

		return pBuffer;
	}
	_VIRT_V FreePacket(ReadWritePacket *p) override
	{
		ReleaseBuffer(p);
	}
	_VIRT_H WritePacket(USHORT AppChannelId, char* buff, ULONG size, CUserData* pUser/* = nullptr*/) override
	{
		ASSERT(size > 0 && size <= 64*1024*1024);
		CAppChl* pAppChannel = LockChannel(AppChannelId, pUser);
		if ( pAppChannel)
		{
			HRESULT hr = WritePacket(pAppChannel, buff, size);
			UnlockChannel(pAppChannel);
			return hr;
		}
		return E_INVALIDARG;
	}
	_VIRT_H WritePacket(CAppChl *pAppChn, char* buff, ULONG size) override;
	_VIRT_H WritePacket(CAppChl *pAppChn, ReadWritePacket *pPacket) override
	{
		BOOL bRet = SendPacket(((CAppChlEx*)pAppChn)->pTransChannel, pPacket, pAppChn->Level);
		return bRet ? S_OK : E_FAIL;
	}

	_VIRT_B CreateDynamicChannel(CAppChl* pAppChannel
		 , EmPacketLevel level = Middle, USHORT option = NULL) override// CAppChl::uPacketOption option
	{
		CAppChlEx* p = createDynamicChannel(pAppChannel, level, option);
		if ( p)
		{
			p->Unlock();
			return TRUE;
		}
		return FALSE;
	}

	_VIRT_V	CloseDynamicChannel(CAppChl* pDynamicChannel) override;
	_VIRT_H		CloseDynamicChannel(USHORT AppChannelId, CUserData* pUser/* = nullptr*/) override
	{
		CAppChl* pAppChannel = LockChannel(AppChannelId, pUser);
		if ( pAppChannel)
		{
			CloseDynamicChannel(pAppChannel);
			UnlockChannel(pAppChannel);
			return S_OK;
		}
		return E_NOTFOUND;
	}

private://internal function
	CAppChl* createCrossAppChannel(CAppChl* pStaticChannel, CTransferChannelEx* pTransChn);
	CAppChlEx* createDynamicChannel(CAppChl* pAppChannel, EmPacketLevel level, USHORT option);
	CUserDataEx* findUser(DWORD dwSessionId)
	{
		if ( dwSessionId == (DWORD)-1/* && (!UserName || !UserName[0])*/)
		{
			ASSERT(0);
			return nullptr;
		}

		CUserDataEx* pFind = nullptr;
		LOCK(&m_smapUser);
		POSITION pos = m_smapUser.GetStartPosition();
		while (pos)
		{
			CUserDataEx* pGet = m_smapUser.GetNextValue(pos);
			if ( dwSessionId == pGet->dwSessionId)
			{
				pFind = pGet;
				break;
			}
		}

		return pFind;
	}
	CUserDataEx* findUser(WCHAR UserName[260])
	{
		if ( !UserName || !UserName[0])
		{
			ASSERT(0);
			return nullptr;
		}

		CUserDataEx* pFind = nullptr;
		LOCK(&m_smapUser);
		POSITION pos = m_smapUser.GetStartPosition();
		while (pos)
		{
			CUserDataEx* pGet = m_smapUser.GetNextValue(pos);
			if ( pGet->dwSessionId != (DWORD)-1)
				continue;

			if ( !_wcsicmp(pGet->wsUserName, UserName))
			{
				pFind = pGet;
				break;
			}
		}

		return pFind;
	}

	void	closeStaticChannel(CAppChl* pStaticChannel);

	inline void closeAppChannel(CAppChl* pAppChannel);			// close static channel.

	CAppChlEx* allocateAppChannel(CUserDataEx *user
		, ICTEPAppProtocol* piAppProt, LPCSTR sAppName
		, CAppChlEx *pStaAppChn = nullptr
		, EmPacketLevel level = Middle, USHORT option = 0
		, CTransferChannelEx* pTransChn = nullptr);
	void releaseAppChannel(CAppChlEx* pAppChnEx);

	CUserDataEx* allocateUser(CTransferChannelEx* pTransChnMain);
	void releaseUser(CUserDataEx* pUserEx);

	inline ReadWritePacket*	MallocPacket(CAppChl *pAppChn, ULONG size)// 创建发送包内存
	{
		return AllocateBuffer(((CAppChlEx*)pAppChn)->pTransChannel, EmPacketOperationType::OP_IocpSend, size);
	}

private:// Event Impl
	_VIRT_B OnStart() override;
	_VIRT_V OnShutdown() override;

	_VIRT_V OnConnectionEstablished(CTransferChannelEx *pContext, ReadWritePacket *pBuffer) override;
	_VIRT_V OnReadCompleted(CTransferChannelEx *pContext, ReadWritePacket *pBuffer) override;
	_VIRT_V OnConnectionClosing(CTransferChannelEx *pContext, ReadWritePacket *pBuffer) override;
#ifdef _DEBUG
	_VIRT_B OnConnectionError(CTransferChannelEx *pContext, ReadWritePacket *pBuffer, int nError) override
	{
		UNREFERENCED_PARAMETER(pBuffer);
		m_log.FmtWarning(5, L" 一个连接发生错误[0x%08x]-Socket(%d) ErrCode:%d[0x%08x] \n ", pContext, (DWORD)pContext->hFile, nError, nError);
		return FALSE;// 返回FALSE表示不处理
	}
#endif // _DEBUG
};
































