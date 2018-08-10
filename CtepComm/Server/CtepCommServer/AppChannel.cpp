#include "stdafx.h"

#include "AppChannel.h"


// class CCtepCommunicationServer
HRESULT CCtepCommunicationServer::WritePacket(CAppChannel *pAppChn, char* buff, ULONG size)
{
	if ( !pAppChn || pAppChn->bClosing || !pAppChn->pUser || ((CUserDataEx*)pAppChn->pUser)->Status == User_Invalid)
		return E_FAIL;

	if ( !buff || size == 0)
		return E_INVALIDARG;

	CUserDataEx* pUser = (CUserDataEx*)pAppChn->pUser;
	USHORT UserId = pUser->UserId;

	if ( size <= CTEP_DEFAULT_BUFFER_DATA_SIZE)
	{
		ReadWritePacket* pPacket = MallocPacket(pAppChn, 0);
		if ( !pPacket)
		{
			ASSERT(0);
			return E_OUTOFMEMORY;
		}
		pPacket->buff.size = Create_CTEPPacket_Message((CTEPPacket_Message*)pPacket->buff.buff
			, UserId, pAppChn->AppChannelId, buff, (USHORT)size);

		return WritePacket(pAppChn, pPacket);
	}

	DWORD dwSizeLast = size;
	while ( dwSizeLast > 0)
	{
		USHORT dwPacketLength = (USHORT)min(CTEP_DEFAULT_BUFFER_DATA_SIZE, dwSizeLast);
		ReadWritePacket* pPacket = MallocPacket(pAppChn, 0);
		if ( !pPacket)
		{
			ASSERT(0);
			return E_OUTOFMEMORY;
		}

		// 填写头
		if ( dwSizeLast == size)
		{
			pPacket->buff.size = Create_CTEPPacket_Message((CTEPPacket_Message*)pPacket->buff.buff
				, UserId, pAppChn->AppChannelId, buff, dwPacketLength
				, size, true, false);
		}
		else if ( dwSizeLast <= CTEP_DEFAULT_BUFFER_DATA_SIZE)
		{
			pPacket->buff.size = Create_CTEPPacket_Message((CTEPPacket_Message*)pPacket->buff.buff
				, UserId, pAppChn->AppChannelId, buff+(size-dwSizeLast), dwPacketLength
				, size, false, true);
		}
		else
		{
			pPacket->buff.size = Create_CTEPPacket_Message((CTEPPacket_Message*)pPacket->buff.buff
				, UserId, pAppChn->AppChannelId, buff+(size-dwSizeLast), dwPacketLength
				, size, false, false);
		}

		if ( FAILED(WritePacket(pAppChn, pPacket)))
			return E_FAIL;

		dwSizeLast -= dwPacketLength;
	}

	return S_OK;
}


	// CAppChannel::uPacketOption option
CAppChannel* CCtepCommunicationServer::CreateDynamicChannel(CAppChannel* pStaticChannel
	, EmPacketLevel level /*= Middle*/, USHORT option /*= NULL*/)
{
	CAppChannelEx *pAppChnEx = (CAppChannelEx*)pStaticChannel;
	CAppChannelEx *pStaAppChnEx = pAppChnEx->pStaticAppChannel;
	CUserDataEx* pUserEx = (CUserDataEx*)pAppChnEx->pUser;

	ASSERT(pAppChnEx && pStaAppChnEx && pUserEx);
	if ( !pAppChnEx || !pStaAppChnEx || !pUserEx || pUserEx->Status == User_Invalid)
		return nullptr;

	CAppChannelEx *pNewAppChannel = allocateAppChannel((CUserDataEx*)pAppChnEx->pUser
		, pAppChnEx->piAppProtocol, pStaAppChnEx->sAppName
		, pAppChnEx->pStaticAppChannel, level, option);

	// 发送客户端应用层动态通道建立消息
	if ( pNewAppChannel)
	{
		ReadWritePacket* pPacketSend = MallocPacket(pNewAppChannel, sizeof(CTEPPacket_CreateAppRsp));
		if ( pPacketSend)
		{
			pPacketSend->buff.size = Create_CTEPPacket_CreateAppRsp(
				(CTEPPacket_CreateAppRsp*)pPacketSend->buff.buff, pUserEx->UserId
				, pNewAppChannel->AppChannelId
				, 0
				, pStaticChannel->piAppProtocol->GetName()
				, option, (USHORT)level
				, pStaticChannel->AppChannelId
				, TRUE);

			HRESULT hr = WritePacket(pNewAppChannel, pPacketSend);
			ASSERT(SUCCEEDED(hr));

			LOCK(pNewAppChannel);
			pAppChnEx->piAppProtocol->Connect(pAppChnEx->pUser, pNewAppChannel, pAppChnEx->pStaticAppChannel);
		}
		else
		{
			m_log.Error(3, L"ConnectEstablished - MallocPacket Failed. No Enough Memory.");
		}
	}

	return pNewAppChannel;
}

void	CCtepCommunicationServer::CloseDynamicChannel(CAppChannel* pDynamicChannel)
{
	CAppChannelEx *pAppChnEx = (CAppChannelEx*)pDynamicChannel;
	ASSERT(pAppChnEx);
	if ( !pAppChnEx || pAppChnEx == pAppChnEx->pStaticAppChannel)
		return ;

	// 发送客户端应用程序动态通道删除消息
	CUserDataEx* pUserEx = (CUserDataEx*)pAppChnEx->pUser;
	if ( pUserEx && pUserEx->Status == User_Connected)
	{
		ReadWritePacket* pBuffer = MallocPacket(pDynamicChannel, sizeof(CTEPPacket_CloseAppRsp));
		if ( pBuffer)
		{
			pBuffer->buff.size = Create_CTEPPacket_Header((CTEPPacket_Header*)pBuffer->buff.buff
				, sizeof(CTEPPacket_CloseAppRsp), CTEP_PACKET_CONTENT_CLOSE_APP_RSP
				, pUserEx->UserId, pDynamicChannel->AppChannelId);

			WritePacket(pDynamicChannel, pBuffer);
		}
	}

	if ( pAppChnEx->Type == CrossDyncChannel)
	{
		// 断开跨进程通信管道
		//X;

	}

	releaseAppChannel(pAppChnEx);
}

void CCtepCommunicationServer::closeStaticChannel(CAppChannel* pStaticChannel)
{
	CUserDataEx* pUserEx = (CUserDataEx*)pStaticChannel->pUser;
	CAppChannelEx *pStaticChn = ((CAppChannelEx*)pStaticChannel)->pStaticAppChannel;
	ASSERT( pStaticChn);

	ASSERT(((CAppChannelEx*)pStaticChannel)->pStaticAppChannel == pStaticChannel
		&& pStaticChannel->Type == StaticChannel);

	// 关闭指定静态Channel以及Channel之下的所有动态Channel
	CAppChannelEx *pDynamicChn = pStaticChn->PopLink();
	while( pDynamicChn)
	{
		CloseDynamicChannel(pDynamicChn);
		pDynamicChn = pStaticChn->PopLink();
	}

	// 发送客户端应用层静态通道删除消息
	if ( pUserEx && pUserEx->Status == User_Connected)
	{
		ReadWritePacket* pBuffer = MallocPacket(pStaticChannel, sizeof(CTEPPacket_CloseAppRsp));
		if ( pBuffer)
		{
			pBuffer->buff.size = Create_CTEPPacket_Header((CTEPPacket_Header*)pBuffer->buff.buff
				, sizeof(CTEPPacket_CloseAppRsp), CTEP_PACKET_CONTENT_CLOSE_APP_RSP
				, pUserEx->UserId, pStaticChannel->AppChannelId);

			WritePacket(pStaticChannel, pBuffer);
		}
	}

	releaseAppChannel((CAppChannelEx*)pStaticChannel);
}

void CCtepCommunicationServer::closeAppChannel(CAppChannel* pAppChannel)			// close static channel.
{
	if ( !pAppChannel)
	{
		ASSERT(0);
		return ;
	}

	if ( pAppChannel->Type == DynamicChannel || pAppChannel->Type == CrossDyncChannel)
	{
		CloseDynamicChannel(pAppChannel);
		return ;
	}
	else
	{
		closeStaticChannel(pAppChannel);
		return ;
	}
}

CAppChannelEx* CCtepCommunicationServer::allocateAppChannel(CUserDataEx *user, ICTEPAppProtocol* piAppProt, LPCSTR sAppName
	, CAppChannelEx *pStaAppChn /*= nullptr*/, EmPacketLevel level /*= Middle*/, USHORT option /*= 0*/)
{
	ASSERT(user && piAppProt && sAppName && sAppName[0]);
	if ( level > High)
		level = Middle;
	else if ( level < Low)
		level = Low;

	CAppChannelEx* pNewAppChannel = m_queFreeAppChn.Pop();
	if ( !pNewAppChannel)
		pNewAppChannel = new CAppChannelEx();
	
	if ( pNewAppChannel)
	{
		pNewAppChannel->Initialize(user, piAppProt, sAppName, pStaAppChn, level, option);

		pNewAppChannel->AppChannelId = m_smapAppChn.Push(pNewAppChannel);

		if ( pNewAppChannel->AppChannelId == m_smapAppChn.InvalidCount())
		{
			releaseAppChannel(pNewAppChannel);
			pNewAppChannel = nullptr;
		}
	}

	return pNewAppChannel;
}
void CCtepCommunicationServer::releaseAppChannel(CAppChannelEx* pAppChnEx)
{
	ASSERT(pAppChnEx);
	if ( pAppChnEx->AppChannelId != m_smapAppChn.InvalidCount())
	{
		if ( pAppChnEx->piAppProtocol)
		{
			LOCK(pAppChnEx);
			if ( pAppChnEx->piAppProtocol)
			{
				ICTEPAppProtocol* pTempAppProt = pAppChnEx->piAppProtocol;
				pAppChnEx->piAppProtocol = nullptr;
				pTempAppProt->Disconnect(pAppChnEx->pUser, pAppChnEx);
			}
		}

		pAppChnEx = m_smapAppChn.Pop(pAppChnEx->AppChannelId);
	}

	if ( pAppChnEx)
	{
		pAppChnEx->Recycling();
		m_queFreeAppChn.Push(pAppChnEx);
	}
}

CUserDataEx* CCtepCommunicationServer::allocateUser(CTransferChannelEx* pTransChnMain)
{
	ASSERT(pTransChnMain->pUser == nullptr);
	CUserDataEx* pUserEx = m_queFreeUserData.Pop();
	if ( !pUserEx)
		pUserEx = new CUserDataEx;

	pUserEx->Initialize(pTransChnMain, nullptr);
	pUserEx->UserId = m_smapUser.Push(pUserEx);
	if ( pUserEx->UserId == m_smapUser.InvalidCount())
	{
		releaseUser(pUserEx);
		return nullptr;
	}

	return pUserEx;
}

void CCtepCommunicationServer::releaseUser(CUserDataEx* pUserEx)
{
	ASSERT( pUserEx);
	ASSERT( !pUserEx->pTransChnMain && !pUserEx->pTransChnTcp && !pUserEx->pTransChnUdp);
	ASSERT( 0 == pUserEx->collAppStaticChannel.GetCount());
	if ( pUserEx->UserId != (USHORT)-1)
	{
		pUserEx = m_smapUser.Pop(pUserEx->UserId);
	}

	if ( pUserEx)
	{
		pUserEx->Recycling();
		m_queFreeUserData.Push(pUserEx);
	}
}



void CCtepCommunicationServer::OnConnectionEstablished(CTransferChannelEx *pContext, ReadWritePacket *pBuffer)
{
	m_log.FmtMessage(5, L" 接收到一个新的连接[0x%08x] Type:[%s] Handle:%d.  All Connection Count（%d）： %S \n"
		, pContext, pContext->debugEnTransfaerChannelType()
		, (DWORD)pContext->hFile, GetCurrentConnection(), ::inet_ntoa(pContext->addrRemote.sin_addr));
	CUserDataEx *pUserEx = nullptr;
	CTEPPacket_Init* pInit = nullptr;
	if ( pBuffer && pBuffer->buff.size > 0)
	{
		ASSERT(pBuffer->buff.size == sizeof(CTEPPacket_Init));
		if ( pBuffer->buff.size != sizeof(CTEPPacket_Init))
		{
			ASSERT(0);
			goto ErrorEnd;
		}
		pInit = (CTEPPacket_Init*)pBuffer->buff.buff;
		if ( !pInit->IsPacketInit())
		{
			pInit = nullptr;
			ASSERT(0);
			goto ErrorEnd;
		}
	}

	if ( pContext->type == TransType_TCP)
	{
		if (pBuffer->buff.size != sizeof(CTEPPacket_Init))
		{
			ASSERT(pBuffer->buff.size < sizeof(CTEPPacket_Init));
			goto ErrorEnd;
		}

		if ( !pInit)
			goto ErrorEnd;

		if ( pInit->header.GetUserId() == (USHORT)-1)
		{
			ASSERT(IsEqualGUID(GUID_NULL, pInit->guidUserSession));
			pUserEx = allocateUser(pContext);
			if ( !pUserEx)
			{
				m_log.Error(3, L"ConnectEstablished - allocateUser Failed.");
				goto ErrorEnd;
			}
			m_log.print(L"TCP Make a new User: %d", pUserEx->UserId);
		} 
		else
		{
			pUserEx = m_smapUser.Find(pInit->header.GetUserId());
			if ( !pUserEx)
			{
				m_log.Error(3, L"ConnectEstablished Failed. UserData Not Found.");
				goto ErrorEnd;
			}

			ASSERT(pUserEx->pTransChnTcp == nullptr);
			if ( pUserEx->pTransChnTcp != nullptr)
			{
				m_log.Error(3, L"ConnectEstablished Failed. Tcp Channel has Connected.");
				goto ErrorEnd;
			}

			GUID guidServer;
			memcpy(&guidServer, &pInit->guidUserSession, sizeof(GUID));
			if ( !IsEqualGUID(guidServer, pUserEx->guidUser))
			{
				m_log.Error(3, L"ConnectEstablished Failed. Guid not match.");
				goto ErrorEnd;
			}

			pUserEx->Lock();
			pUserEx->pTransChnTcp = pContext;
			pContext->pUser = pUserEx;
			pUserEx->Unlock();
			m_log.print(L"Tcp with old User: %d", pUserEx->UserId);
		}
	}
	else if ( pContext->type == TransType_UDP)
	{
		ASSERT(0);// Not Implement
	}
	else// 主通道
	{
		ASSERT(pBuffer->buff.size == 0 ||
			!strcmp(pContext->piTrans->GetName(), "CTVP"));

		if ( pContext->dwSessionId != (DWORD)-1)
		{
			pUserEx = findUser(pContext->dwSessionId);
			if ( pUserEx)
			{
				ASSERT(!pUserEx->pTransChnMain && !pUserEx->pTransChnTcp && !pUserEx->pTransChnUdp && pUserEx->Status == User_Disconnected);
				if ( pUserEx->Status != User_Disconnected)
				{
					ASSERT(0);
					goto ErrorEnd;
				}

				pUserEx->pTransChnMain = pContext;
				pUserEx->pTransChnTcp = nullptr;
				pUserEx->pTransChnUdp = nullptr;
				pUserEx->Status = User_Connected;
			}
		}

		if ( !pUserEx)
			pUserEx = allocateUser(pContext);
	}

	if ( pInit)
	{
		// 回复Init Response
		ASSERT(pUserEx && pUserEx->pTransChnMain);
		ReadWritePacket* pPacketSend = AllocateBuffer(pContext, OP_IocpSend);
		if ( !pPacketSend)
		{
			m_log.Error(3, L"ConnectEstablished - AllocateBuffer Failed. No Enough Memory.");
			goto ErrorEnd;
		}

		pPacketSend->buff.size = Create_CTEPPacket_Init_Responce(
			(CTEPPacket_Init_Responce*)pPacketSend->buff.buff
			, pUserEx->UserId, pUserEx->dwSessionId, pUserEx->guidUser, pUserEx->wsUserName
			, m_TS[0] ? m_TS[0]->GetPort() : 0, m_TS[1] ? m_TS[1]->GetPort() : 0
			, m_LocalIPv4, m_nIPv4Count);

		SendPacket(pContext, pPacketSend, High);
	}
	return ;

ErrorEnd:
	m_log.Error(5, L"OnConnectionEstablished Failed!");
	CloseAConnection(pContext);
}

void CCtepCommunicationServer::OnConnectionClosing(CTransferChannelEx *pContext, ReadWritePacket *pBuffer)
{
	UNREFERENCED_PARAMETER(pBuffer);
	CUserDataEx* pUserEx = (CUserDataEx*)pContext->pUser;
	pContext->pUser = nullptr;

	m_log.FmtMessage(5, L" 一个连接关闭:[0x%08x] Handle:%d. User:[%d]  All Connection Count（%d）： %S \n"
		, pContext, (DWORD)pContext->hFile, pUserEx ? pUserEx->UserId : -1
		, GetCurrentConnection(), ::inet_ntoa(pContext->addrRemote.sin_addr));

	if ( !pUserEx || pUserEx->Status == User_Invalid)
	{
		return ;
	}

	if ( pUserEx->Status == User_Disconnected)
	{
		if ( pContext->type == TransType_CrossApp)
		{
			//X;
		}

		return ;
	}

	LOCK(pUserEx);
	if ( pUserEx->Status != User_Connected)
		return ;

	pUserEx->Status = User_Disconnected;

	// 关闭所有底层连接
	if ( pUserEx->pTransChnTcp)
	{
		CloseAConnection(pUserEx->pTransChnTcp);
		//if ( pContext == pUserEx->pTransChnTcp)
		pUserEx->pTransChnTcp = nullptr;
	}
	if ( pUserEx->pTransChnUdp)
	{
		CloseAConnection(pUserEx->pTransChnUdp);
		//if ( pContext == pUserEx->pTransChnUdp)
		pUserEx->pTransChnUdp = nullptr;
	}
	if ( pUserEx->pTransChnMain)
	{
		CloseAConnection(pUserEx->pTransChnMain);
		//if ( pContext == pUserEx->pTransChnMain)
		pUserEx->pTransChnMain = nullptr;
	}

	// 关闭该用户打开的所有应用
	{
		pUserEx->collAppStaticChannel.Lock();

		CAppChannel** listAppChannel;
		int iCount = pUserEx->collAppStaticChannel.GetValueList(&listAppChannel);
		for ( int i = 0; i < iCount; i++)
		{
			CAppChannelEx* pAppChannel = (CAppChannelEx*)listAppChannel[i];
			ASSERT(pAppChannel && pAppChannel->AppChannelId != (USHORT)-1);
			if ( pAppChannel->QueryDisconnect())
			{
				pUserEx->DeleteApp(pAppChannel->AppChannelId);
				closeAppChannel(pAppChannel);
			}
		}
		pUserEx->collAppStaticChannel.ReleaseValueList(listAppChannel);

		pUserEx->collAppStaticChannel.Unlock();
	}

	if ( pUserEx->collAppStaticChannel.IsEmpty())
	{
		pUserEx->Status = User_Invalid;
		releaseUser(pUserEx);
	}
}

void CCtepCommunicationServer::OnReadCompleted(CTransferChannelEx *pContext, ReadWritePacket *pBuffer)
{
	ASSERT(pContext && pBuffer);
	if ( !pContext || !pBuffer)
		return ;

	m_log.FmtMessage(1, L"收到一个数据包[0x%08x] . 大小:%d", pContext, pBuffer->buff.size);

	CUserDataEx* pUserEx = (CUserDataEx*)pContext->pUser;
	if ( !pUserEx || pUserEx->Status == User_Invalid)
		return ;

	CTEPPacket_Header* pHeader;
	while ( nullptr != (pHeader = pContext->SplitPacket(pBuffer)))
	{
		if ( pHeader->IsHello())
		{
			ReadWritePacket *pPacketSend = AllocateBuffer(
				pContext, OP_IocpSend, sizeof(CTEPPacket_Hello));
			if ( pPacketSend)
			{
				Create_CTEPPacket_Hello((CTEPPacket_Hello*)pPacketSend->buff.buff, (LPCWSTR)(pHeader+1), TRUE);
				SendPacket(pContext, pPacketSend, High);
			}
			m_log.FmtMessageW(3, L"OnReadCompleted - IsHello(). %s", ((CTEPPacket_Hello*)pHeader)->msg);
			continue;
		}
		if ( pHeader->IsHelloRsp())
		{
			m_log.FmtMessageW(3, L"OnReadCompleted - IsHelloRsp(). %s", ((CTEPPacket_Hello*)pHeader)->msg);
			continue;
		}

		if ( pHeader->IsInit())
		{
			// 回复Init Response
			ASSERT(pUserEx && pUserEx->pTransChnMain);
			m_log.print(L"New Client Init. UserId:%d, TransChannelHandle:%d, Type:[%S]",pUserEx->UserId, pContext->hFile
				, pContext->piTrans->GetName());

			ReadWritePacket* pPacketSend = AllocateBuffer(
				pUserEx->pTransChnMain, OP_IocpSend);
			if ( !pPacketSend)
			{
				m_log.Error(3, L"OnReadCompleted - AllocateBuffer Failed. No Enough Memory.");
				goto ErrorEnd;
			}

			pPacketSend->buff.size = Create_CTEPPacket_Init_Responce(
				(CTEPPacket_Init_Responce*)pPacketSend->buff.buff
				, pUserEx->UserId, pUserEx->dwSessionId, pUserEx->guidUser, pUserEx->wsUserName
				, m_TS[0] ? m_TS[0]->GetPort() : 0, m_TS[1] ? m_TS[1]->GetPort() : 0
				, m_LocalIPv4, m_nIPv4Count);

			SendPacket(pContext, pPacketSend, High);
			continue;
		}

		CAppChannelEx* pAppChnEx = m_smapAppChn.Find(pHeader->GetAppId());
		if ( pAppChnEx && pAppChnEx->pUser != (CUserData*)pUserEx)
		{
			ASSERT(pAppChnEx->bClosing && pAppChnEx->pUser == nullptr);// 找到的通道不属于当前用户
			continue;
		}

		if ( !pAppChnEx && pHeader->IsMessage())// 找不到指定的通道,可能已经被关闭了
		{
			m_log.FmtWarning(3, L"OnReadCompleted - [IsMessage] AppId:[%d] no found.", pHeader->GetAppId());
			// 发送关闭回复
			ReadWritePacket* pPacketSend = AllocateBuffer(pContext, EmPacketOperationType::OP_IocpSend, 0);
			if ( pPacketSend)
			{
				pPacketSend->buff.size = Create_CTEPPacket_Header(
					(CTEPPacket_Header*)pPacketSend->buff.buff
					, sizeof(CTEPPacket_CloseAppRsp), CTEP_PACKET_CONTENT_CLOSE_APP_RSP
					, pUserEx->UserId, pHeader->GetAppId());

				SendPacket(pContext, pPacketSend, High);
			}
			else
			{
				m_log.Error(3, L"OnReadCompleted - AllocateBuffer Failed. No Enough Memory.");
			}
			continue;
		}

		if ( pHeader->IsCloseApp())
		{
			// 发送关闭回复
			ReadWritePacket* pPacketSend = AllocateBuffer(pContext, EmPacketOperationType::OP_IocpSend, 0);
			if ( pPacketSend)
			{
				pPacketSend->buff.size = Create_CTEPPacket_Header(
					(CTEPPacket_Header*)pPacketSend->buff.buff
					, sizeof(CTEPPacket_CloseAppRsp), CTEP_PACKET_CONTENT_CLOSE_APP_RSP
					, pUserEx->UserId, pHeader->GetAppId());

				if ( pAppChnEx)
				{
					ASSERT(pAppChnEx->pTransChannel);
					SendPacket(pAppChnEx->pTransChannel, pPacketSend, pAppChnEx->Level);
				}
				else
				{
					SendPacket(pContext, pPacketSend, Middle);
				}
			}
			else
			{
				m_log.Error(3, L"OnReadCompleted - MallocPacket Failed. No Enough Memory.");
			}

			if ( pAppChnEx)
			{
				closeAppChannel(pAppChnEx);
			}
			continue;
		}

		if ( pHeader->GetAppId() != (USHORT)-1)
		{
			ASSERT(pAppChnEx && pAppChnEx->AppChannelId == pHeader->GetAppId());
			if ( !pAppChnEx)
				continue;
		}

		if ( pHeader->IsCreateApp())
		{
			CTEPPacket_CreateApp* pPacketRecv = (CTEPPacket_CreateApp*)pHeader;

			CAppChannelEx*		pNewAppChannel		= nullptr;
			ICTEPAppProtocol*	piApp				= nullptr;
			LPCSTR				sAppName			= nullptr;

			for ( DWORD i=0; i< m_AppCount; i++)
			{
				ICTEPAppProtocol* piRetrieve = m_APP[i];
				ICTEPAppProtocolEx* piRetrieveEx = nullptr;

				ASSERT(piRetrieve);
				if ( !piRetrieve)
					continue;

				if ( piRetrieve->GetInterfaceVersion() == 0)
				{
					if ( !_stricmp(piRetrieve->GetName(), pPacketRecv->AppName))
					{
						piApp = piRetrieve;
						sAppName = piApp->GetName();
						ASSERT(sAppName && sAppName[0] && piApp);
						break;
					}
				}
				else if ( piRetrieve->GetInterfaceVersion() == 1)
				{
					piRetrieveEx = dynamic_cast<ICTEPAppProtocolEx*>(piRetrieve);
					ASSERT(piRetrieveEx);
					DWORD dwIndexCount = piRetrieveEx->GetNameCount();
					for ( DWORD j = 0; j < dwIndexCount; j++)
					{
						if ( !_stricmp(piRetrieveEx->GetNameIndex(j), pPacketRecv->AppName))
						{
							piApp = piRetrieveEx;
							sAppName = piRetrieveEx->GetNameIndex(j);
							ASSERT(sAppName && sAppName[0] && piApp);

							break;
						}
					}
				}
				else
				{
					ASSERT(0);
				}
			}

			if ( piApp && sAppName)
			{
				if ( !pAppChnEx)	// 如果没有主AppChanEx,说明这是创建一个主App通道,否则需要创建一个附属App通道
				{
					ASSERT( pPacketRecv->ePacketLevel == EmPacketLevel::Middle);
					ASSERT( !((pPacketRecv->uPacketOption&(~Packet_Need_Encryption))));
					pPacketRecv->ePacketLevel = EmPacketLevel::Middle;
					pPacketRecv->uPacketOption &= Packet_Need_Encryption;
				}

				pNewAppChannel = (CAppChannelEx*)
					allocateAppChannel(pUserEx, piApp, sAppName, pAppChnEx
					, (EmPacketLevel)pPacketRecv->ePacketLevel, pPacketRecv->uPacketOption);
			}

			// 发送创建回复
			ReadWritePacket* pPacketSend = AllocateBuffer(pContext, EmPacketOperationType::OP_IocpSend, 0);
			if ( pPacketSend)
			{
				HRESULT hr = E_FAIL;
				pPacketSend->buff.size = Create_CTEPPacket_CreateAppRsp(
					(CTEPPacket_CreateAppRsp*)pPacketSend->buff.buff, pUserEx->UserId
					, pNewAppChannel ? pNewAppChannel->AppChannelId : (USHORT)-1
					, pPacketRecv->Key
					, pPacketRecv->AppName
					, pPacketRecv->uPacketOption
					, pPacketRecv->ePacketLevel
					, pHeader->GetAppId()
					, pNewAppChannel ? TRUE : FALSE);

				if ( pNewAppChannel)
				{
					hr = SendPacket(pNewAppChannel->pTransChannel, pPacketSend, pNewAppChannel->Level);
					pNewAppChannel->piAppProtocol->Connect(pUserEx, pNewAppChannel, pAppChnEx);
					m_log.print(L" [UID:%d][TYPE:%S] App Created:[%S] AppId:%d{%s}"
						, pUserEx->UserId, pContext->piTrans->GetName()
						, pPacketRecv->AppName, pNewAppChannel->AppChannelId, pNewAppChannel->debugType());
				}
				else
				{
					hr = SendPacket(pContext, pPacketSend, High);
					m_log.FmtWarning(3, L" [UID:%d][TYPE:%S] Unsupport App:[%S]"
						, pUserEx->UserId, pContext->piTrans->GetName()
						, pPacketRecv->AppName);
				}
				ASSERT(SUCCEEDED(hr));

				m_log.print(L"OnReadCompleted - CreateApp:[%S]. hResult:[%d] AppChannelId:[%d] - with StaticAppChannelId:[%d]"
					, pPacketRecv->AppName, hr
					, pNewAppChannel ? pNewAppChannel->AppChannelId : (USHORT)-1
					, pHeader->GetAppId());
			}
			else
			{
				m_log.Error(3, L"OnReadCompleted - AllocateBuffer Failed. No Enough Memory.");
			}

			continue;
		}

		if ( pHeader->IsMessage())
		{
			CTEPPacket_Message* pMsg = (CTEPPacket_Message*)pHeader;

			DWORD dwSize = 0;
			char* buff = pAppChnEx->SplitPacket(pMsg, dwSize);
			if ( buff && buff != (char*)-1)
			{
				if ( pAppChnEx->piAppProtocol)
				{
					pAppChnEx->Lock();
					pAppChnEx->piAppProtocol->ReadPacket(pUserEx, pAppChnEx, buff, dwSize);
					pAppChnEx->Unlock();

					pAppChnEx->SplitPacket(0, dwSize);
				}
			}
			else if ( buff == (char*)-1)
			{
				closeAppChannel(pAppChnEx);
			}

			continue;
		}

		ASSERT(0);
	}

	return ;

ErrorEnd:
	m_log.Error(5, L"OnReadCompleted Failed!");
	CloseAConnection(pContext);
}

BOOL CCtepCommunicationServer::OnStart()
{
	BOOL bRet = FALSE;
	for ( DWORD i=0; i< m_AppCount; i++)
	{
		if ( m_APP[i])
		{
			HRESULT hr = m_APP[i]->Initialize(dynamic_cast<ICTEPAppProtocolCallBack*>(this), CTEP_TYPE_APP_SERVER);
			if ( SUCCEEDED(hr))
			{
				m_log.print(L"App[%S] initialize success.", m_APP[i]->GetName());
				bRet = TRUE;
			}
			else
			{
				m_log.FmtError(5, L"App[%S] initialize failed. ErrCode:0x%08x"
					, m_APP[i]->GetName(), hr);
				m_APP[i]->Final();
				m_APP[i] = 0;
			}
		}
	}
	if ( !bRet)
	{
		m_log.FmtError(5, L"App initialize failed.");
	}
	else
	{
		// 启动消息回调处理线程:
		m_ThreadCallBack.StartThread((LPVOID)-1);
	}

	return bRet;
}

void CCtepCommunicationServer::OnShutdown()
{
	// 关闭消息回调处理线程
	m_ThreadCallBack.StopThread();

	// 关闭所有应用连接
	CAppChannelEx* pApp;
	while(NULL != (pApp = m_smapAppChn.FindFirst()))
	{
		closeAppChannel(pApp);
	}

	// 通知所有应用关闭
	for ( DWORD i=0; i< m_AppCount; i++)
	{
		if ( m_APP[i])
		{
			m_APP[i]->Final();
		}
	}

	// 关闭所有用户
	//X;
}











