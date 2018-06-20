#pragma once

#include "CommonInclude/Tools/SerialNumColloction.h"

class CCtepCommClient : public ICTEPTransferProtocolClientCallBack, public CLoadModules
{
public:
	CCtepCommClient():m_UserEx(0, GUID_NULL), m_bWorking(FALSE), m_log("CommClt")
	{
		m_queFreePacket.Initialize(true, true, true);
		FindMoudleInterface(CLoadModules::CtepTransClient);
		FindMoudleInterface(CLoadModules::CtepAppClient);
	}

	HRESULT Initalize(ICTEPTransferProtocolClient* piTransProtClt, const sockaddr* soa, int size)
	{
		if ( !piTransProtClt)
			return E_INVALIDARG;

		if ( m_AppCount == 0)
			return E_NOTIMPL;

		if ( m_bWorking)
		{
			ASSERT(0);
			return S_FALSE;
		}

		StTransferChannelEx *pTransMain = nullptr;
		HRESULT hr = piTransProtClt->Initialize( this);
		if ( FAILED(hr))
			goto End;

		pTransMain = AllocateContext(INVALID_HANDLE_VALUE, piTransProtClt);
		ASSERT(pTransMain);
		m_UserEx.pTransChnMain = pTransMain;
		if ( soa && size > 0)
		{
			memcpy(&pTransMain->head.addrRemote, soa, size);
		}

		ReadWritePacket* pPacketInit = AllocatePacketListen(piTransProtClt);
		pPacketInit->buff.size = Create_CTEPPacket_Init((CTEPPacket_Init*)pPacketInit->buff.buff);

		hr = piTransProtClt->Connect((StTransferChannel*)pTransMain, pPacketInit);
		ASSERT( SUCCEEDED(hr));
		ReleasePacket(pPacketInit);

		if ( FAILED(hr))
			goto End;

		m_bWorking = TRUE;
		// 通知上层启动
		OnStart();

		if ( pTransMain->head.type == TCP)
		{
			DoStart(pTransMain);
		}

		End:

		return hr;
	}

public:// interface ICTEPTransferProtocolClientCallBack
	virtual HRESULT Connected(StTransferChannel* pTransChn) override
	{
		m_log.print(L"Connected. [0x%08x]-%S"
			, pTransChn
			, pTransChn->piTransClt->GetName());

		return S_OK;
	}
	virtual HRESULT Disconnected(StTransferChannel* pTransChn, DWORD dwErr) override
	{
		m_log.print(L"Disconnected. [0x%08x]-%S"
			, pTransChn
			, pTransChn->piTransClt->GetName());

		return S_OK;
	}
	virtual HRESULT Recv(StTransferChannel* pTransChn, ReadWritePacket* pPacket) override
	{
		// 处理收到的数据
		StTransferChannelEx *pTransChnEx = (StTransferChannelEx*)pTransChn;
		CTEPPacket_Header* pHeader = pTransChnEx->SplitPacket(pPacket);
		while ( pHeader)
		{
			if ( pHeader->IsInitRsp())
			{
				CTEPPacket_Init_Responce *pInitRsp = (CTEPPacket_Init_Responce*)pHeader;
				if ( pTransChnEx == m_UserEx.pTransChnMain)
				{
					ASSERT(m_UserEx.UserId == (USHORT)-1 && m_UserEx.guidUser == GUID_NULL);
					m_UserEx.UserId = pInitRsp->header.GetUserId();
					m_UserEx.guidUser = pInitRsp->guidUserSession;
					wcscpy_s(m_UserEx.wsUserName, pInitRsp->wsUserName);
					if ( pTransChn->type == OTHER)
					{
						// 启动TCP处理线程和UDP处理线程
						if ( m_TC[0])//TCP
						{
							BOOL bConnectTcp = TRUE;
							// 试图连接TCP


							// 启动新线程接管TCP连接


							// 如果启动失败,则枚举应用
							if ( !bConnectTcp)
							{
								OnAppRetireve( pTransChnEx);
							}
						}

						if ( m_TC[1])//UDP
						{

						}
					}
					else if ( pTransChn->type == TCP)
					{
						// 枚举App,发送App Connect.
						OnAppRetireve( pTransChnEx);
					}
					else
					{
						ASSERT(0);
					}
				}
				else if ( pTransChnEx == m_UserEx.pTransChnTcp)
				{
					// 枚举App,发送App Connect.
					OnAppRetireve(pTransChnEx);
				}
				else if ( pTransChnEx == m_UserEx.pTransChnUdp)
				{
					ASSERT(0);
				}
				else
				{
					ASSERT(0);
				}
			}
			else
			{
				OnReadCompleted(pTransChnEx, pHeader);
			}
			pHeader = pTransChnEx->SplitPacket(pPacket);
		}

		return S_OK;
	}

protected:// Event
	virtual BOOL OnStart() = 0;
	virtual void OnShutdown() = 0;	// 告诉上层关闭
	virtual void OnAppRetireve(StTransferChannelEx* pTransChn) = 0;
	virtual void OnReadCompleted(StTransferChannelEx *pContext, CTEPPacket_Header *pHeader) = 0;

protected: // internel function
	HRESULT DoStart(StTransferChannelEx* pTransChnEx)
	{
		HRESULT hr = S_OK;
		if ( !m_bWorking)
			return E_FAIL;

		ICTEPTransferProtocolClient* piTransClt = pTransChnEx->head.piTransClt;
		ASSERT(piTransClt);

		ReadWritePacket *buffRead = AllocatePacket(&pTransChnEx->head, OP_Recv);
		while( SUCCEEDED(hr) && m_bWorking)
		{
			buffRead->PacketInit();
			buffRead->piTransClt = piTransClt;
			buffRead->opType = EmPacketOperationType::OP_Recv;
			hr = piTransClt->Recv((StTransferChannel*)pTransChnEx, buffRead);
		}

		// 关闭服务器
		ULONG bClose = InterlockedExchange(&m_bWorking, FALSE);

		// 关闭用户应用
		if ( bClose)
		{
			OnShutdown();
		}

		// 关闭所有通道
		if ( m_UserEx.pTransChnMain)
			m_UserEx.pTransChnMain->DisconnectClient();
		if ( m_UserEx.pTransChnTcp)
			m_UserEx.pTransChnTcp->DisconnectClient();
		if ( m_UserEx.pTransChnUdp)
			m_UserEx.pTransChnUdp->DisconnectClient();

		if ( m_UserEx.pTransChnTcp == pTransChnEx)
			m_UserEx.pTransChnTcp = nullptr;
		else if ( m_UserEx.pTransChnUdp == pTransChnEx)
			m_UserEx.pTransChnUdp = nullptr;
		else if ( m_UserEx.pTransChnMain == pTransChnEx)
			m_UserEx.pTransChnMain = nullptr;
		else
		{
			ASSERT(0);
		}

		return S_OK;
	}

	StTransferChannelEx *AllocateContext(HANDLE s, ICTEPTransferProtocolClient* piTrans)
	{
		StTransferChannelEx* pContext = new StTransferChannelEx();
		if( pContext)
		{
			pContext->InitEx();
			pContext->head.hFile = s;
			pContext->head.piTransClt = piTrans;
			pContext->bClosing = FALSE;
			if ( !_stricmp(piTrans->GetName(), "TCP"))
			{
				pContext->head.type = EnTransferChannelType::TCP;
			}
			else if ( !_stricmp(piTrans->GetName(), "UDP"))
			{
				pContext->head.type = EnTransferChannelType::UDP;
			}
			else
			{
				pContext->head.type = EnTransferChannelType::OTHER;
			}
		}
		return pContext;
	}
	void ReleaseContext(StTransferChannelEx *pContext)
	{
		delete pContext;
	}
	ReadWritePacket* AllocatePacketListen(ICTEPTransferProtocolClient *piTransProt)
	{
		ReadWritePacket* p = new ReadWritePacket();
		if ( p)
		{
			p->PacketInit();
			p->piTransClt = piTransProt;
			p->opType = EmPacketOperationType::OP_Listen;
		}
		return p;
	}
	ReadWritePacket* AllocatePacket(StTransferChannel* pContext, EmPacketOperationType opType, DWORD dwSize = 0)
	{
		ReadWritePacket* p = new ReadWritePacket();
		ASSERT(dwSize < CTEP_DEFAULT_BUFFER_SIZE);
		if ( p)
		{
			p->PacketInit();
			p->piTransClt = pContext->piTransClt;
			if ( dwSize != 0)
				p->buff.size = dwSize;
			p->opType = opType;
			p->s = pContext->s;
		}
		return p;
	}
	void ReleasePacket(ReadWritePacket* pPacket)
	{
		delete pPacket;
	}

protected:
	// 记录空闲结构信息
	FastQueue<ReadWritePacket> m_queFreePacket;
	CUserDataEx m_UserEx;

	ULONG volatile m_bWorking;
	Log4CppLib m_log;
};


class CCtepCommClientApp : public CCtepCommClient
	, public ICTEPAppProtocolCallBack
{
public: // Interface ICTEPAppProtocolCallBack
	virtual CAppChannel* LockChannel(USHORT AppChannelId) override
	{
		m_UserEx.Lock();
		CAppChannelEx* pFind = m_smapAppChn.Find(AppChannelId);
		if ( !pFind)
		{
			m_UserEx.Unlock();
		}
		return pFind;
	}
	virtual void UnlockChannel(CAppChannel* pAppChannel) override
	{
		ASSERT(pAppChannel);
		if ( pAppChannel)
		{
			m_UserEx.Unlock();
		}
	}

	virtual HRESULT WritePacket(CAppChannel *pAppChn, char* buff, ULONG size) override
	{
		HRESULT hr = E_FAIL;
		if ( m_UserEx.bClosing)
			return hr;

		if ( !buff || size == 0)
			return E_INVALIDARG;
		ASSERT( size <= 64*1024);	// 测试是否有超大包

		DWORD nPacketCount = size / CTEP_DEFAULT_BUFFER_DATA_SIZE
			+ (size % CTEP_DEFAULT_BUFFER_DATA_SIZE > 0 ? 1 : 0);

		DWORD dwPacketHeader = 
			nPacketCount == 1 ? sizeof(CTEPPacket_Header) : sizeof(CTEPPacket_Message);

		CUserDataEx* pUser = (CUserDataEx*)pAppChn->pUser;

		DWORD dwSizeSent = 0;
		for (DWORD i = 0; i < nPacketCount; i++)
		{
			USHORT dwPacketLength = (USHORT)min(CTEP_DEFAULT_BUFFER_DATA_SIZE, size - dwSizeSent);
			ReadWritePacket* pPacket = MallocPacket(pAppChn, dwPacketLength);
			if ( !pPacket)
			{
				ASSERT(0);
				return E_OUTOFMEMORY;
			}

			// 填写头
			pPacket->buff.size = dwPacketHeader + dwPacketLength;
			if ( nPacketCount == 1)
			{
				Create_CTEPPacket_Message((CTEPPacket_Message*)pPacket->buff.buff
					, pUser->UserId, pAppChn->AppChannelId, buff, dwPacketLength);
			}
			else
			{
				if ( i == 0)
				{
					Create_CTEPPacket_Message((CTEPPacket_Message*)pPacket->buff.buff
						, pUser->UserId, pAppChn->AppChannelId, buff+dwSizeSent, dwPacketLength
						, size, true, false);
				}
				else if ( i == nPacketCount - 1)
				{
					Create_CTEPPacket_Message((CTEPPacket_Message*)pPacket->buff.buff
						, pUser->UserId, pAppChn->AppChannelId, buff+dwSizeSent, dwPacketLength
						, size, false, true);
				}
				else
				{
					Create_CTEPPacket_Message((CTEPPacket_Message*)pPacket->buff.buff
						, pUser->UserId, pAppChn->AppChannelId, buff+dwSizeSent, dwPacketLength
						, size, false, false);
				}
			}

			if ( FAILED(WritePacket(pAppChn, pPacket)))
				return E_FAIL;

			dwSizeSent += dwPacketLength;
		}

		return S_OK;
	}
	virtual HRESULT WritePacket(CAppChannel *pAppChn, ReadWritePacket *pPacket) override
	{
		if ( m_UserEx.bClosing)
			return E_FAIL;
		CAppChannelEx* pAppChnEx = (CAppChannelEx*)pAppChn;
		StTransferChannelEx* pTransEx = (StTransferChannelEx*)pAppChn->pTransChannel;
		if ( pTransEx && pTransEx->head.piTransClt)
		{
			return pTransEx->SendPacketClient(pPacket);
		}

		return E_UNEXPECTED;
	}

	virtual CAppChannel* CreateDynamicChannel(CAppChannel* pStaticChannel
		, EmPacketLevel ePacketLevel = Middle, USHORT uPacketOption = NULL) override
	{
		if ( m_UserEx.bClosing)
			return 0;
		// 向服务器发送创建动态通道的消息
		ReadWritePacket* pPacket = MallocPacket(pStaticChannel, sizeof(CTEPPacket_CreateApp));
		if ( pPacket)
		{
			CTEPPacket_CreateApp* pHeader = (CTEPPacket_CreateApp*)pPacket->buff.buff;
			Create_CTEPPacket_CreateApp(pHeader, m_UserEx.UserId, pStaticChannel->AppChannelId
				, 0, pStaticChannel->piAppProtocol->GetName(), uPacketOption, ePacketLevel);
			
			((StTransferChannelEx*)pStaticChannel->pTransChannel)->SendPacketClient(pPacket);
			ReleasePacket(pPacket);
		}
		return 0;
	}

	virtual void	CloseDynamicChannel(CAppChannel* pDynamicChannel) override
	{
		// 向服务器发送关闭动态通道的消息
		CloseDynamicChannel((StTransferChannelEx*)pDynamicChannel->pTransChannel
			, pDynamicChannel->AppChannelId);
		return ;
	}

	virtual ReadWritePacket* MallocPacket(CAppChannel *pAppChn, ULONG size = 0) override
	{
		if ( pAppChn && pAppChn->pTransChannel && pAppChn->pTransChannel->piTransClt)
			return AllocatePacketListen(pAppChn->pTransChannel->piTransClt);

		return nullptr;
	}
	virtual void FreePacket(ReadWritePacket *pPacket) override
	{
		ReleasePacket(pPacket);
	}


public: // Event
	virtual BOOL OnStart() override
	{
		// 初始化所有App
		for ( DWORD i=0; i<m_AppCount;i++)
		{
			if ( m_APP[i])
			{
				HRESULT hr = m_APP[i]->Initialize((ICTEPAppProtocolCallBack*)this);
				if ( FAILED(hr))
				{
					m_APP[i] = nullptr;
				}
			}
		}

		return TRUE;
	}
	virtual void OnShutdown() override
	{
		// 关闭所有APP
		CAppChannelEx* pAppChnEx = m_smapAppChn.Pop();
		while ( pAppChnEx)
		{
			pAppChnEx->piAppProtocol->Disconnect(&m_UserEx, pAppChnEx);
			delete pAppChnEx;
			pAppChnEx = m_smapAppChn.Pop();
		}

		for ( DWORD i=0; i < m_AppCount; i++)
		{
			if ( m_APP[i])
			{
				m_APP[i]->Final();
			}
		}
	}

	virtual void OnAppRetireve(StTransferChannelEx* pTransChn) override
	{
		// 枚举所有App
		ReadWritePacket* pPacket = AllocatePacket(&pTransChn->head, OP_Send, sizeof(CTEPPacket_CreateApp));
		CTEPPacket_CreateApp* pHeader = (CTEPPacket_CreateApp*)pPacket->buff.buff;
		Create_CTEPPacket_CreateApp(pHeader, m_UserEx.UserId, (USHORT)-1
			, 0, "", 0, EmPacketLevel::Middle);
		pHeader->Key = 0;
		for (DWORD i=0; i < m_AppCount; i++)
		{
			strcpy_s(pHeader->AppName, m_APP[i]->GetName());
			pTransChn->SendPacketClient(pPacket);
		}

		ReleasePacket(pPacket);
	}

	virtual void OnReadCompleted(StTransferChannelEx *pContext, CTEPPacket_Header *pHeader) override
	{
		CAppChannelEx* pAppChnEx = nullptr;
		if ( !pHeader->InvalidAppChannelId())
		{
			pAppChnEx = m_smapAppChn.Find(pHeader->GetAppId());
		}

		if ( pHeader->IsCreateAppRsp())
		{
			CTEPPacket_CreateAppRsp *pRecv = (CTEPPacket_CreateAppRsp*)pHeader;
			CAppChannelEx* pNewAppChn;
			ICTEPAppProtocol * piApp = nullptr;

			for ( DWORD i = 0; i < m_AppCount; i++)
			{
				if ( !_stricmp(m_APP[i]->GetName(), pRecv->AppName))
				{
					piApp = m_APP[i];
					break;
				}
			}

			if ( piApp)
			{
				CAppChannelEx *pStaticApp = m_smapAppChn.Find(pRecv->StaticAppChannelId);
				pNewAppChn = AllocateAppChannel(piApp, pHeader->GetAppId()
					, pStaticApp, (EmPacketLevel)pRecv->ePacketLevel, pRecv->uPacketOption);
			}
			else
			{
				ASSERT(0);
				CloseDynamicChannel(pContext, pHeader->AppChannelId);
			}
		}
		else if ( pHeader->IsCloseAppRsp())
		{
			CTEPPacket_CloseAppRsp *pRecv = (CTEPPacket_CloseAppRsp*)pHeader;





		}
		else if ( pHeader->IsMessage())
		{
			if ( pAppChnEx)
			{
				CTEPPacket_Message* pMsg = (CTEPPacket_Message*)pHeader;

				DWORD dwSize = 0;
				char* buff = pAppChnEx->SplitPacket(pMsg, dwSize);
				if ( buff && buff != (char*)-1)
				{
					pAppChnEx->piAppProtocol->ReadPacket(&m_UserEx
						, pAppChnEx, buff, dwSize);
					pAppChnEx->SplitPacket(0, dwSize);
				}
				else if ( buff == (char*)-1)
				{
					CloseDynamicChannel(pAppChnEx);
				}
			}
			else
			{
				ASSERT(0);
			}
		}
		else
		{
			ASSERT(0);
		}

	}

public:
	void	CloseDynamicChannel(StTransferChannelEx* pTransChnEx, USHORT AppChannelId)
	{
		// 向服务器发送关闭动态通道的消息
		ReadWritePacket* pPacket = AllocatePacket(&pTransChnEx->head, OP_Send, sizeof(CTEPPacket_CloseApp));
		if ( pPacket)
		{
			CTEPPacket_CloseApp* pHeader = (CTEPPacket_CloseApp*)pPacket->buff.buff;
			Create_CTEPPacket_Header((CTEPPacket_Header*)pHeader, sizeof(CTEPPacket_CloseApp)
				, CTEP_PACKET_CONTENT_CLOSE_APP, m_UserEx.UserId, AppChannelId);
			
			pTransChnEx->SendPacketClient(pPacket);
			ReleasePacket(pPacket);
		}
	}

	CAppChannelEx* AllocateAppChannel(ICTEPAppProtocol* piApp, USHORT uAppChnId
		, CAppChannelEx* pAppChnMaster = 0, EmPacketLevel level = Middle, USHORT option = 0)
	{
		CAppChannelEx* pNew;
		LOCK((CMyCriticalSection*)&m_smapAppChn);
		pNew = m_smapAppChn.Find(uAppChnId);
		if ( pNew)
		{
			ASSERT(0);
			return pNew;
		}

		pNew = new CAppChannelEx(&m_UserEx, piApp, pAppChnMaster, level, option);
		if ( pNew)
		{
			pNew->AppChannelId = uAppChnId;
			m_smapAppChn[uAppChnId] = pNew;

			piApp->Connect(&m_UserEx, pNew, pNew->pStaticAppChannel);
		}

		return pNew;
	}
	void ReleaseAppChannel(USHORT uAppChnId)
	{
		CAppChannelEx *pFind = m_smapAppChn.Pop(uAppChnId);
		if ( pFind)
		{
			pFind->pStaticAppChannel->Lock();
			if ( pFind->Type == DynamicChannel)
			{
				pFind->RemoveLink();
			}
			else
			{
				ASSERT(pFind->Type == StaticChannel);
				while ( pFind->pNextDynamicAppChannel)
				{
					CAppChannelEx* pDyn = pFind->pNextDynamicAppChannel;
					ASSERT(pDyn->pStaticAppChannel == pFind);
					pFind->pNextDynamicAppChannel = pDyn->pNextDynamicAppChannel;
					delete pDyn;
				}
			}
			pFind->pStaticAppChannel->Unlock();

			pFind->piAppProtocol->Disconnect(&m_UserEx, pFind);

			delete pFind;
		}
	}


private:
	SerialNumColl<CAppChannelEx*, USHORT> m_smapAppChn;

};








