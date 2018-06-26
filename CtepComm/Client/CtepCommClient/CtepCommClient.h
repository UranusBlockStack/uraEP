#pragma once

#include "CommonInclude/Tools/SerialNumColloction.h"

class CCtepCommClient : public ICTEPTransferProtocolClientCallBack, public CLoadModules
{
public:
	CCtepCommClient():m_UserEx(0, GUID_NULL), m_bWorking(FALSE), m_log("CommClt")
	{
		ReadWritePacket* pBuffer = new ReadWritePacket();
		pBuffer->PacketInit();
		m_queFreePacket.Initialize(pBuffer, true, true, 1);
		for (int i=0; i<50; i++)
		{
			pBuffer = new ReadWritePacket();
			if ( pBuffer)
			{
				pBuffer->PacketInit();
				m_queFreePacket.Push(pBuffer);
			}
			else
			{
				ASSERT(0);
			}
		}

		FindMoudleInterface(CLoadModules::CtepTransClient);
		FindMoudleInterface(CLoadModules::CtepAppClient);
	}
	~CCtepCommClient()
	{

	}

	void Shutdown()
	{
		if ( m_bWorking)
		{
			ASSERT(m_bWorking && m_UserEx.pTransChnMain);
			if ( m_UserEx.pTransChnMain)
			{
				m_UserEx.pTransChnMain->DisconnectClient();
			}
		}
	}

	HRESULT Initalize(ICTEPTransferProtocolClient* piTransProtClt, const sockaddr* soa, int size)
	{
		ASSERT(piTransProtClt && m_AppCount && !m_bWorking && !m_UserEx.pTransChnMain && m_UserEx.bClosing);
		if ( !piTransProtClt)
			return E_INVALIDARG;

		if ( m_AppCount == 0)
			return E_NOTIMPL;

		if ( m_bWorking)
			return S_FALSE;

		m_UserEx.bClosing = FALSE;
		HRESULT hr = piTransProtClt->Initialize( this);
		if ( FAILED(hr))
			goto End;

		CTransferChannelEx* pTransMain = AllocateContext(INVALID_HANDLE_VALUE, piTransProtClt);
		ASSERT(pTransMain);
		m_UserEx.pTransChnMain = pTransMain;
		if ( soa && size > 0)
		{
			memcpy(&pTransMain->addrRemote, soa, size);
		}

		ReadWritePacket* pPacketInit = AllocatePacketListen(piTransProtClt);
		pPacketInit->buff.size = Create_CTEPPacket_Init((CTEPPacket_Init*)pPacketInit->buff.buff);
		hr = piTransProtClt->Connect(pTransMain, pPacketInit);
		if ( hr != ERROR_IO_PENDING)
			ReleasePacket(pPacketInit);

		if ( FAILED(hr))
			goto End;

		if ( m_TC[0])
		{
			m_TC[0]->Initialize(this);
		}
		if ( m_TC[1])
		{
			m_TC[1]->Initialize(this);
		}

		m_bWorking = TRUE;
		// 通知上层启动
		OnStart();

		if ( pTransMain->type == TCP)
		{
			doStart(pTransMain);
			hr = E_FAIL;
		}

End:
		if ( FAILED(hr))
		{
			m_UserEx.bClosing = TRUE;
		}

		return hr;
	}

public:// interface ICTEPTransferProtocolClientCallBack
	virtual HRESULT Connected(CTransferChannel* pTransChn) override
	{
		m_log.print(L"Connected. [0x%08x]-%S"
			, pTransChn
			, pTransChn->piTransClt->GetName());

		return S_OK;
	}
	virtual HRESULT Disconnected(CTransferChannel* pTransChn, DWORD dwErr) override
	{
		m_log.print(L"Disconnected. [0x%08x]-%S"
			, pTransChn
			, pTransChn->piTransClt->GetName());

		return S_OK;
	}

	bool connectTcp(CTEPPacket_Init_Responce * pInitRsp, CTransferChannelEx* pTransChnExTcp, SOCKADDR_IN *addr)
	{
		if ( !m_TC[0])//TCP
			return false;

		// 试图连接TCP
		HRESULT hr = E_FAIL;
		ASSERT(!m_UserEx.pTransChnTcp && pTransChnExTcp->piTransClt == m_TC[0]);
		ReadWritePacket* pPacket = AllocatePacket(pTransChnExTcp, OP_IocpSend, sizeof(CTEPPacket_Init));
		Create_CTEPPacket_Init((CTEPPacket_Init*)pPacket->buff.buff, m_UserEx.UserId, pInitRsp->guidUserSession);
		memcpy(&pTransChnExTcp->addrRemote, addr, sizeof(SOCKADDR_IN));
		hr = m_TC[0]->Connect(pTransChnExTcp, pPacket);
		ASSERT(hr == S_OK);
		ReleasePacket(pPacket);
		if ( FAILED(hr))
		{
			return false;
		}

		pPacket = AllocatePacket(pTransChnExTcp, OP_IocpRecv);
		pPacket->buff.buff = pTransChnExTcp->rbufPacket.GetBuffer(&pPacket->buff.maxlength);
		hr = m_TC[0]->Recv(pTransChnExTcp, pPacket);
		ReleasePacket(pPacket);
		if ( hr == S_OK && !pTransChnExTcp->bClosing)
		{
			m_UserEx.pTransChnTcp = pTransChnExTcp;
			pTransChnExTcp->pUser = &m_UserEx;
			return true;
		}

		return false;
	}

	virtual HRESULT Recv(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override
	{
		CTransferChannelEx *pTransChnEx = (CTransferChannelEx*)pTransChn;

		if ( !pPacket)
		{
			if ( pTransChnEx->piTransClt == m_TC[0] && !pTransChnEx->pUser)
			{
				ASSERT(m_UserEx.pTransChnTcp == 0);
				pTransChnEx->bClosing = TRUE;
				return E_FAIL;
			}

			doClose(pTransChnEx);
			return -1;
		}

		m_log.FmtMessage(1, L"收到一个数据包.大小:%d", pPacket->buff.size);
		// 处理收到的数据
		if ( pTransChnEx->rbufPacket.m_pDataEnd != pPacket->buff.buff)
		{
			ASSERT(pTransChnEx->type == SyncMain && pPacket->buff.buff && pPacket->buff.size > 0);
			DWORD dwLength = 0;
			char* buffRead = pTransChnEx->rbufPacket.GetBuffer(&dwLength);
			ASSERT(dwLength >= pPacket->buff.size);
			memcpy(buffRead, pPacket->buff.buff, pPacket->buff.size);
			pPacket->buff.buff = buffRead;
		}

		CTEPPacket_Header* pHeader = pTransChnEx->SplitPacket(pPacket);
		while ( pHeader && pHeader != (CTEPPacket_Header*)-1)
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
					if ( pTransChnEx->type == TCP)
					{
						// 枚举App,发送App Connect.
						OnAppRetireve( pTransChnEx);
					}
					else if ( pTransChnEx->type != TCP && pTransChnEx->type != UDP)
					{
						// 启动TCP处理线程和UDP处理线程
						BOOL bConnectTcp = FALSE;
						do 
						{
							if ( m_TC[0])//TCP
							{
								CTransferChannelEx* pTransChnExTcp = AllocateContext(INVALID_HANDLE_VALUE, m_TC[0]);
								pTransChnEx->addrRemote.sin_port = htons(pInitRsp->TcpPort);
								bConnectTcp = connectTcp(pInitRsp, pTransChnExTcp, &pTransChnEx->addrRemote);

								if ( !bConnectTcp)
								{
									for (DWORD i=0; !bConnectTcp && i<pInitRsp->IPv4Count; i++)
									{
										pTransChnEx->addrRemote.sin_addr = pInitRsp->IPv4[i];
										bConnectTcp = connectTcp(pInitRsp, pTransChnExTcp, &pTransChnEx->addrRemote);
									}
								}

								// 启动新线程接管TCP连接
								if ( bConnectTcp)
								{
									HANDLE hThread = CreateThread(0, 0, _threadTcpChannel, this, 0, 0);
									ASSERT(hThread);
									CloseHandle(hThread);
								}
							}

						} while (0);

						if ( m_TC[1])//UDP
						{
							ASSERT(0);
						}

						// 如果启动失败,则枚举应用
						if ( !bConnectTcp)
						{
							OnAppRetireve( pTransChnEx);
						}
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
					if ( pTransChnEx->piTransClt == m_TC[0] && !pTransChnEx->pUser)
					{
						ASSERT(m_UserEx.pTransChnTcp == 0);
						OnAppRetireve(pTransChnEx);
					}
					else
					{
						ASSERT(0);
					}
				}
			}
			else
			{
				ASSERT( pHeader->UserId == m_UserEx.UserId);
				OnReadCompleted(pTransChnEx, pHeader);
			}
			pHeader = pTransChnEx->SplitPacket(pPacket);
		}

		return S_OK;
	}
	virtual ReadWritePacket* AllocatePacket(ICTEPTransferProtocolClient* pI) override
	{
		ReadWritePacket* p = m_queFreePacket.Pop();
		if ( !p)
			p = new ReadWritePacket();

		if ( p)
		{
			p->PacketInit();
			p->piTransClt = pI;
			p->opType = OP_IocpRecv;
		}

		return p;
	}

	virtual void FreePacket(ReadWritePacket* pPacket) override
	{
		ReleasePacket(pPacket);
	}

protected:// Event
	virtual BOOL OnStart() = 0;
	virtual void OnShutdown() = 0;	// 告诉上层关闭
	virtual void OnAppRetireve(CTransferChannelEx* pTransChn) = 0;
	virtual void OnReadCompleted(CTransferChannelEx *pContext, CTEPPacket_Header *pHeader) = 0;

protected: // internel function
	void doClose(CTransferChannelEx* pTransChnEx)
	{
		ASSERT(pTransChnEx);

		m_UserEx.Lock();
		if ( !m_UserEx.bClosing)
		{
			m_UserEx.bClosing = TRUE;
			m_UserEx.guidUser = GUID_NULL;
			m_UserEx.UserId = -1;
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
		m_UserEx.Unlock();
	}

	HRESULT doStart(CTransferChannelEx* pTransChnEx)
	{
		HRESULT hr = S_OK;
		if ( !m_bWorking)
			return E_FAIL;

		ICTEPTransferProtocolClient* piTransClt = pTransChnEx->piTransClt;
		ASSERT(piTransClt);

		ReadWritePacket *buffRead = AllocatePacket(pTransChnEx, OP_IocpRecv);
		while( SUCCEEDED(hr) && m_bWorking)
		{
			buffRead->PacketInit();
			buffRead->piTransClt = piTransClt;
			buffRead->opType = EmPacketOperationType::OP_IocpRecv;
			buffRead->buff.buff = pTransChnEx->rbufPacket.GetBuffer(&buffRead->buff.maxlength);
			hr = piTransClt->Recv(pTransChnEx, buffRead);
		}

		doClose(pTransChnEx);

		return S_OK;
	}

	CTransferChannelEx *AllocateContext(HANDLE s, ICTEPTransferProtocolClient* piTrans)
	{
		CTransferChannelEx* pContext = new CTransferChannelEx();
		if( pContext)
		{
			pContext->hFile = s;
			pContext->piTransClt = piTrans;
			pContext->bClosing = FALSE;
			if ( !_stricmp(piTrans->GetName(), "TCP"))
			{
				pContext->type = EnTransferChannelType::TCP;
			}
			else if ( !_stricmp(piTrans->GetName(), "UDP"))
			{
				pContext->type = EnTransferChannelType::UDP;
			}
			else
			{
				pContext->type = EnTransferChannelType::SyncMain;
			}
		}
		return pContext;
	}
	void ReleaseContext(CTransferChannelEx *pContext)
	{
		delete pContext;
	}
	ReadWritePacket* AllocatePacketListen(ICTEPTransferProtocolClient *piTransProt)
	{
		ReadWritePacket* p = m_queFreePacket.Pop();
		if ( !p)
			p = new ReadWritePacket();

		if ( p)
		{
			p->PacketInit();
			p->piTransClt = piTransProt;
			p->opType = EmPacketOperationType::OP_IocpSend;
		}

		return p;
	}
	ReadWritePacket* AllocatePacket(CTransferChannel* pContext, EmPacketOperationType opType, DWORD dwSize = 0)
	{
		ReadWritePacket* p = m_queFreePacket.Pop();
		if ( !p)
			p = new ReadWritePacket();

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
		m_queFreePacket.Push(pPacket);
	}

	static DWORD WINAPI _threadTcpChannel(LPVOID param)
	{
		CCtepCommClient* pThis = (CCtepCommClient*)param;
		ASSERT(pThis->m_UserEx.pTransChnTcp);
		return pThis->doStart(pThis->m_UserEx.pTransChnTcp);
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
public:
	CCtepCommClientApp()
	{
		m_queFreeAppChn.Initialize(new CAppChannelEx(), true, true, 50);
	}

public:
// Interface ICTEPAppProtocolCallBack
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
		{
			return E_NOINTERFACE;
		}

		if ( !buff || size == 0)
			return E_INVALIDARG;

		ASSERT( size <= 64*1024);	// 测试是否有超大包

		DWORD nPacketCount = size / CTEP_DEFAULT_BUFFER_DATA_SIZE
			+ (size % CTEP_DEFAULT_BUFFER_DATA_SIZE > 0 ? 1 : 0);

		DWORD dwPacketHeader = 
			nPacketCount == 1 ? sizeof(CTEPPacket_Header) : sizeof(CTEPPacket_Message);

		CUserDataEx* pUser = (CUserDataEx*)pAppChn->pUser;
		CTransferChannelEx* pTransChn = ((CAppChannelEx*)pAppChn)->pTransChannel;

		DWORD dwSizeSent = 0;
		for (DWORD i = 0; i < nPacketCount; i++)
		{
			USHORT dwPacketLength = (USHORT)min(CTEP_DEFAULT_BUFFER_DATA_SIZE, size - dwSizeSent);
			ReadWritePacket* pPacket = AllocatePacket(pTransChn, OP_IocpSend, dwPacketLength);
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

			HRESULT hr = WritePacket(pAppChn, pPacket);
			ASSERT(SUCCEEDED(hr));
			if ( FAILED(hr))
			{
				return E_FAIL;
			}

			dwSizeSent += dwPacketLength;
		}

		return S_OK;
	}

	virtual HRESULT WritePacket(CAppChannel *pAppChn, ReadWritePacket *pPacket) override
	{
		if ( m_UserEx.bClosing)
			return E_NOINTERFACE;

		CAppChannelEx* pAppChnEx = (CAppChannelEx*)pAppChn;
		CTransferChannelEx* pTransEx = pAppChnEx->pTransChannel;
		if ( pTransEx && pTransEx->piTransClt)
		{
			HRESULT hr = pTransEx->SendPacketClient(pPacket);
			ASSERT(SUCCEEDED(hr));
			if ( hr != ERROR_IO_PENDING)
			{
				ReleasePacket(pPacket);
			}
			return hr;
		}

		return E_UNEXPECTED;
	}

	virtual CAppChannel* CreateDynamicChannel(CAppChannel* pStaticChannel
		, EmPacketLevel ePacketLevel = Middle, USHORT uPacketOption = NULL) override
	{
		CAppChannelEx* pAppChnEx = (CAppChannelEx*)pStaticChannel;
		if ( m_UserEx.bClosing)
			return 0;
		// 向服务器发送创建动态通道的消息
		ReadWritePacket* pPacket = AllocatePacket(pAppChnEx->pTransChannel
			, OP_IocpSend, sizeof(CTEPPacket_CreateApp));
		if ( pPacket)
		{
			CTEPPacket_CreateApp* pHeader = (CTEPPacket_CreateApp*)pPacket->buff.buff;
			Create_CTEPPacket_CreateApp(pHeader, m_UserEx.UserId, pStaticChannel->AppChannelId
				, 0, pStaticChannel->piAppProtocol->GetName(), uPacketOption, ePacketLevel);
			
			HRESULT hr = (((CAppChannelEx*)pStaticChannel)->pTransChannel)->SendPacketClient(pPacket);
			if ( hr != ERROR_IO_PENDING)
			{
				ReleasePacket(pPacket);
			}
		}
		return 0;
	}

	virtual void	CloseDynamicChannel(CAppChannel* pDynamicChannel) override
	{
		// 向服务器发送关闭动态通道的消息
		CloseDynamicChannel(((CAppChannelEx*)pDynamicChannel)->pTransChannel
			, pDynamicChannel->AppChannelId);
		return ;
	}

	virtual ReadWritePacket* MallocSendPacket(CAppChannel *pAppChn, USHORT size) override
	{
		ASSERT(size <= CTEP_DEFAULT_BUFFER_DATA_SIZE);
		if ( size > CTEP_DEFAULT_BUFFER_DATA_SIZE)
			return nullptr;

		CUserData *pUser = ((CAppChannelEx*)pAppChn)->pUser;
		ASSERT(pUser == (CUserData *)&m_UserEx);
		ReadWritePacket* pBuffer = nullptr;
		if (	pAppChn
			 && ((CAppChannelEx*)pAppChn)->pTransChannel
			 && ((CAppChannelEx*)pAppChn)->pTransChannel->piTransClt)
		{
			pBuffer = AllocatePacket(((CAppChannelEx*)pAppChn)->pTransChannel, OP_IocpSend, 0);
		}

		if ( pBuffer)
		{
			CTEPPacket_Message* pMsg = (CTEPPacket_Message*)pBuffer->buff.buff;
			pBuffer->buff.size = Create_CTEPPacket_Message(pMsg, pUser->UserId, pAppChn->AppChannelId, 0, size);

			pBuffer->pointer = pMsg->GetBuffer();
		}

		return pBuffer;
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
				HRESULT hr = m_APP[i]->Initialize((ICTEPAppProtocolCallBack*)this, 0);
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
		CAppChannelEx* pAppChnEx = m_smapAppChn.FindFirst();
		while ( pAppChnEx)
		{
			ReleaseAppChannel(pAppChnEx);
			pAppChnEx = m_smapAppChn.FindFirst();
		}

		for ( DWORD i=0; i < m_AppCount; i++)
		{
			if ( m_APP[i])
			{
				m_APP[i]->Final();
			}
		}
	}

	virtual void OnAppRetireve(CTransferChannelEx* pTransChn) override
	{
		for (DWORD i=0; i < m_AppCount; i++)
		{
			// 枚举所有App
			ReadWritePacket* pPacket = AllocatePacket(pTransChn, OP_IocpSend, sizeof(CTEPPacket_CreateApp));
			CTEPPacket_CreateApp* pHeader = (CTEPPacket_CreateApp*)pPacket->buff.buff;
			Create_CTEPPacket_CreateApp(pHeader, m_UserEx.UserId, (USHORT)-1
				, 0, "", 0, EmPacketLevel::Middle);
			pHeader->Key = 0;

			strcpy_s(pHeader->AppName, m_APP[i]->GetName());

			HRESULT hr = pTransChn->SendPacketClient(pPacket);
			if ( hr != ERROR_IO_PENDING)
			{
				ReleasePacket(pPacket);
			}
		}
	}

	virtual void OnReadCompleted(CTransferChannelEx *pContext, CTEPPacket_Header *pHeader) override
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
			ReleaseAppChannel(pRecv->header.AppChannelId);
		}
		else if ( pHeader->IsMessage())
		{
			CTEPPacket_Message* pMsg = (CTEPPacket_Message*)pHeader;
			if ( pAppChnEx && pAppChnEx->piAppProtocol)
			{
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
				m_log.FmtWarning(3, L"OnReadCompleted - IsMessage() - Invalid App Channel Id. [%d]", pHeader->GetAppId());
				//ASSERT(0);
			}
		}
		else
		{
			ASSERT(0);
		}

	}

public:
	void	CloseDynamicChannel(CTransferChannelEx* pTransChnEx, USHORT AppChannelId)
	{
		// 向服务器发送关闭动态通道的消息
		ReadWritePacket* pPacket = AllocatePacket(pTransChnEx, OP_IocpSend, sizeof(CTEPPacket_CloseApp));
		if ( pPacket)
		{
			CTEPPacket_CloseApp* pHeader = (CTEPPacket_CloseApp*)pPacket->buff.buff;
			Create_CTEPPacket_Header((CTEPPacket_Header*)pHeader, sizeof(CTEPPacket_CloseApp)
				, CTEP_PACKET_CONTENT_CLOSE_APP, m_UserEx.UserId, AppChannelId);
			
			HRESULT hr = pTransChnEx->SendPacketClient(pPacket);
			if ( hr != ERROR_IO_PENDING)
			{
				ReleasePacket(pPacket);
			}
		}
	}

	CAppChannelEx* AllocateAppChannel(ICTEPAppProtocol* piApp, USHORT uAppChnId
		, CAppChannelEx* pAppChnMaster = 0, EmPacketLevel level = Middle, USHORT option = 0)
	{
		CAppChannelEx* pNewAppChannel;
		pNewAppChannel = m_smapAppChn.Find(uAppChnId);
		if ( pNewAppChannel)
		{
			ASSERT(0);
			return pNewAppChannel;
		}

		pNewAppChannel = m_queFreeAppChn.Pop();
		if ( !pNewAppChannel)
		{
			pNewAppChannel = new CAppChannelEx();
		}

		if ( pNewAppChannel)
		{
			pNewAppChannel->Initialize(&m_UserEx, piApp, pAppChnMaster, level, option);

			pNewAppChannel->AppChannelId = uAppChnId;
			m_smapAppChn.Lock();
			m_smapAppChn[uAppChnId] = pNewAppChannel;
			m_smapAppChn.Unlock();

			piApp->Connect(&m_UserEx, pNewAppChannel, pNewAppChannel->pStaticAppChannel);
		}

		return pNewAppChannel;
	}
	void ReleaseAppChannel(CAppChannelEx* pAppChnEx)
	{
		if ( !pAppChnEx)
			return ;


		if ( pAppChnEx->Type == DynamicChannel)
		{
			pAppChnEx->RemoveLink();
		}
		else
		{
			LOCK((CMyCriticalSection*)pAppChnEx);
			ASSERT(pAppChnEx->Type == StaticChannel);
			while ( pAppChnEx->pNextDynamicAppChannel)
			{
				CAppChannelEx* pDyn = pAppChnEx->pNextDynamicAppChannel;
				ASSERT(pDyn->pStaticAppChannel == pAppChnEx);
				pAppChnEx->pNextDynamicAppChannel = pDyn->pNextDynamicAppChannel;

				pDyn = m_smapAppChn.Pop(pDyn->AppChannelId);
				if ( pDyn)
				{
					pDyn->Recycling();
					m_queFreeAppChn.Push(pDyn);
				}
			}
		}

		if ( pAppChnEx->piAppProtocol)
		{
			pAppChnEx->piAppProtocol->Disconnect(&m_UserEx, pAppChnEx);
			pAppChnEx->piAppProtocol = nullptr;
		}

		pAppChnEx = m_smapAppChn.Pop(pAppChnEx->AppChannelId);
		if ( pAppChnEx)
		{
			pAppChnEx->Recycling();
			m_queFreeAppChn.Push(pAppChnEx);
		}

	}
	void ReleaseAppChannel(USHORT uAppChnId)
	{
		CAppChannelEx *pFind = m_smapAppChn.Pop(uAppChnId);
		ReleaseAppChannel(pFind);
	}


private:
	SerialNumColl<CAppChannelEx*, USHORT> m_smapAppChn;

	FastQueue<CAppChannelEx>   m_queFreeAppChn;
};








