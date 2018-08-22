#pragma once

#include <Rpc.h>
#pragma comment(lib, "Rpcrt4")

#include "Log4Cpp_Lib/Log4CppLib.h"
#include "CommonInclude/DynamicBuffer.h"
#include "CommonInclude/Tools/LockedContainer.h"

#include "CTEP_Communicate_TransferLayer_Interface.h"
#include "CTEP_Communicate_App_Interface.h"

struct CallBackList
{
	CallBackList(StCallEvent::EmEventType inType, DWORD inMaxLength = 32)	// 设置最大可以存储的回调个数
	{
		type = inType;
		lstMaxLength = inMaxLength;
		lstCall = new StCallEvent[inMaxLength];
		ZeroMemory(lstCall, sizeof(StCallEvent) * inMaxLength);
		lstCount = 0;
	}
	~CallBackList()
	{
		delete [] lstCall;
	}

	HRESULT Insert(const StCallEvent& lstCall)
	{
		ASSERT(lstCall.type == type);
		return Insert(lstCall.pParam, lstCall.fnBase);
	}
	HRESULT Insert(void* pParam, Call_BaseFunction fn)
	{
		LOCK(&lck);
		if ( lstCount < lstMaxLength)
		{
			lstCall[lstCount].type = type;
			lstCall[lstCount].pParam = pParam;
			lstCall[lstCount].fnBase = fn;
			lstCount++;
			return S_OK;
		}

		return E_OUTOFMEMORY;
	}
	HRESULT Remove(Call_BaseFunction fn)
	{
		LOCK(&lck);
		for (DWORD i = 0; i < lstCount; i++)
		{
			if ( lstCall[i].fnBase == fn)
			{
				lstCall[i] = lstCall[lstCount - 1];
				ZeroObject(lstCall[lstCount - 1]);
				lstCount--;

				return S_OK;
			}
		}

		return E_NOTFOUND;
	}

public:
	CMyCriticalSection			lck;

	StCallEvent*				lstCall;
	DWORD						lstCount;
	DWORD						lstMaxLength;
	StCallEvent::EmEventType	type;
};

// 这是per-Handle数据。它包含了一个套节字的信息
class __declspec(novtable) CTransferChannelEx : public CTransferChannel
{
	Log4CppLib m_log;
public:
	CTransferChannelEx() : m_log("TransChnEx")
	{
		rbufPacket.Init(cPacketBuf, sizeof(cPacketBuf)-4, sizeof(CTEPPacket_Header)
			, CTEP_DEFAULT_BUFFER_SIZE, CDynamicRingBuffer::defaultGetBagSize);
		bNeedNotificationClosing = FALSE;
		InitEx();
	}
	void InitEx()
	{
		ASSERT( !bNeedNotificationClosing);
		this->Init();
		rbufPacket.Clear();

		nOutstandingSend = 0;
		bNeedNotificationClosing = FALSE;
		pNext = NULL;

		SequenceIdLocal = 0;
		SequenceIdRemote = 0;
	}
	inline HRESULT DisconnectServer(ReadWritePacket* pBuffer = 0)
	{
		ASSERT(piTrans);
		if ( !piTrans)
			return E_FAIL;

		return piTrans->Disconnect(this, pBuffer);
	}
	inline HRESULT DisconnectClient()
	{
		ASSERT(piTransClt);
		if ( !piTransClt)
			return E_FAIL;

		return piTransClt->Disconnect(this);
	}
	inline HRESULT SendPacketClient(ReadWritePacket* pBuffer)
	{
		ASSERT( pBuffer->buff.size <= CTEP_DEFAULT_BUFFER_SIZE);
		::EnterCriticalSection(&lckSend);
		SequenceIdLocal++;
		((CTEPPacket_Header*)pBuffer->buff.buff)->SequenceId = SequenceIdLocal;
		HRESULT hr = piTransClt->Send(this, pBuffer);
		::LeaveCriticalSection(&lckSend);
		return hr;
	}
	inline HRESULT SendPacketServer(ReadWritePacket* pBuffer)
	{
		::EnterCriticalSection(&lckSend);
		ASSERT( pBuffer->buff.size <= CTEP_DEFAULT_BUFFER_SIZE);
		SequenceIdLocal++;
		((CTEPPacket_Header*)pBuffer->buff.buff)->SequenceId = SequenceIdLocal;
		HRESULT hr = piTrans->PostSend(this, pBuffer);
		::LeaveCriticalSection(&lckSend);
		return hr;
	}

	int PreSplitPacket(ReadWritePacket* pPacket)
	{
		int dwPacketSize = 0;

		// 获取pPacket中剩余字节数
#ifdef _DEBUG
		if ( pPacket->buff.size == 0)
		{
			DWORD *pdwMagic = (DWORD*)rbufPacket.m_pDataEnd;
			ASSERT(*pdwMagic == 0x12345678);
		}
		else
		{
			ASSERT(pPacket->buff.buff == rbufPacket.m_pDataEnd);
		}
#endif // _DEBUG
		
		dwPacketSize = (LONG32)rbufPacket.PushData(pPacket->buff.size);
		ASSERT(dwPacketSize >= 0);

#ifdef _DEBUG
		CTEPPacket_Header* pHeader = (CTEPPacket_Header*)rbufPacket.m_pData;
		ASSERT(pHeader->PacketLength == dwPacketSize || dwPacketSize == 0);
		DWORD *pdwMagic = (DWORD*)rbufPacket.m_pDataEnd;
		*pdwMagic = 0x12345678;
#endif // _DEBUG

		pPacket->buff.size = 0;
		return dwPacketSize;
	}
	
	CTEPPacket_Header* SplitPacket(ReadWritePacket* pPacket)
	{
		CTEPPacket_Header* pHeader = nullptr;
		int dwPacketSize = 0;

		// 获取pPacket中剩余字节数
#ifdef _DEBUG
		if ( pPacket->buff.size == 0)
		{
			DWORD *pdwMagic = (DWORD*)rbufPacket.m_pDataEnd;
			ASSERT(*pdwMagic == 0x12345678);
		}
		else
		{
			ASSERT(pPacket->buff.buff == rbufPacket.m_pDataEnd);
		}
#endif // _DEBUG
		dwPacketSize = (LONG32)rbufPacket.PushDataAndPop(pPacket->buff.size, (void**)&pHeader);
		pPacket->buff.size = 0;

		if ( dwPacketSize < 0)
			return (CTEPPacket_Header*)-1;
		else if ( dwPacketSize == 0)
			return nullptr;

#ifdef _DEBUG
		ASSERT(pHeader->PacketLength == dwPacketSize);
		DWORD *pdwMagic = (DWORD*)rbufPacket.m_pDataEnd;
		*pdwMagic = 0x12345678;
#endif // _DEBUG

		if ( SequenceIdRemote == 0 || pHeader->SequenceId == 0 || pHeader->SequenceId == 1)
			SequenceIdRemote = pHeader->SequenceId;
		else
			SequenceIdRemote++;
		if ( SequenceIdRemote != pHeader->SequenceId)
		{
			m_log.FmtError(0x7, L"SequenceIdRemote:%d != pHeader->SequenceId:%d"
				, SequenceIdRemote, pHeader->SequenceId);
			ASSERT(0);
			SequenceIdRemote = pHeader->SequenceId;
		}

		return pHeader;
	}

public:
	volatile ULONG SequenceIdLocal;
	volatile ULONG SequenceIdRemote;

	volatile long nOutstandingSend;// 此套节字上抛出的重叠操作的数量

	volatile UINT bNeedNotificationClosing;	// 是否需要通知App用户退出, TRUE:需要通知, FALSE:不需要通知

	// 保存未完成的一个底层分包
	char				cPacketBuf[CTEP_DEFAULT_BUFFER_SIZE * 32];
	CDynamicRingBuffer	rbufPacket;

	CTransferChannelEx *pNext;
};


class CUserDataEx : public CUserData
	, public CMyCriticalSection
{
public:
	explicit CUserDataEx() : pTransChnMain(nullptr)
		, pTransChnTcp(nullptr), pTransChnUdp(nullptr)
	{
		pNext = nullptr;
	}

	void Recycling()
	{
		ASSERT(0 == collAppStaticChannel.GetCount());	// User回收的时候要保证其上的所有AppChannel都正确关闭了
		ASSERT(!pTransChnMain);							// 保证所有连接已经被关闭
		ASSERT(!pTransChnTcp);
		ASSERT(!pTransChnUdp);

		Status = User_Invalid;
		UserId = INVALID_UID;
	}

	void Initialize(CTransferChannelEx* pMain, const GUID* pGuid = nullptr)
	{
		ASSERT(Status == User_Invalid && pNext == nullptr);
		ASSERT(!pTransChnMain);							// 保证所有连接已经被关闭
		ASSERT(!pTransChnTcp);
		ASSERT(!pTransChnUdp);

		pNext = nullptr;
		pTransChnMain = pTransChnTcp = pTransChnUdp = nullptr;
		Init();

		if ( pMain)
		{
			pTransChnMain = pMain;
			pMain->pUser = this;
			Status = User_Connected;
		}
		else
		{
			Status = User_Disconnected;
		}

		if ( pGuid)
		{
			guidUser = *pGuid;
		}
		else
		{
			UuidCreate(&guidUser);
		}
	}

	CAppChannel* FindApp(USHORT AppChannelId)
	{
		CAppChannel* pFind = collAppStaticChannel.Lookup(AppChannelId);
		ASSERT( pFind->Type == StaticChannel);
		return pFind;
	}

	BOOL AddApp(CAppChannel* pAppChannel)
	{
		USHORT AppId = pAppChannel->AppChannelId;
		ASSERT(pAppChannel && AppId != INVALID_ACID);
		return collAppStaticChannel.SetAt(AppId, pAppChannel);
	}

	CAppChannel* DeleteApp(USHORT AppChannelId)
	{
		return collAppStaticChannel.RemoveKey(AppChannelId);
	}

public:
	CLockMap<USHORT, CAppChannel> collAppStaticChannel;

	CTransferChannelEx* pTransChnMain;
	CTransferChannelEx* pTransChnTcp;
	CTransferChannelEx* pTransChnUdp;

	CUserDataEx *pNext;
};

class CAppChannelEx : public CAppChannel, public CMyCriticalSection
{
public:
	CAppChannelEx()
	{
		sAppName = nullptr;
		pAppParam = nullptr;
		pStaticAppChannel = pNextDynamicAppChannel = nullptr;
		bufData.buff = nullptr;
		pNext = nullptr;
	}
	~CAppChannelEx()
	{
		Recycling();
	}

	void Recycling()
	{
		LOCK(this);

		bClosing = TRUE;
		AppChannelId = INVALID_ACID;
		StaticACId = INVALID_ACID;
		TargetACId = INVALID_ACID;

		RemoveLink();

// 		sAppName = nullptr;
// 		pStaticAppChannel = pNextDynamicAppChannel = nullptr;
// 		pUser = nullptr;
// 		ASSERT(pAppParam == nullptr);
// 
// 		AppChannelId = INVALID_ACID;
// 		piAppProtocol = nullptr;
// 		Level = Low;
// 		uPacketOption = 0;
// 		pTransChannel = nullptr;
// 		nCount = 0;
	}

	void InitializeCrossApp(CUserDataEx *inUserEx, CTransferChannelEx* inpTransChnEx
		, ICTEPAppProtocol* inpiAppProt, LPCSTR inAppName
		, CAppChannelEx *inStaAppChn)
	{
		ASSERT(pNext == NULL && pAppParam == nullptr);
		ASSERT(this && inpiAppProt && inStaAppChn != this);
		Init();

		sAppName = inAppName;
		pStaticAppChannel = inStaAppChn;
		pUser = inUserEx;
		dwInterfaceVer = inpiAppProt->GetInterfaceVersion();
		piAppProtocol = inpiAppProt;
		Level = Middle;
		uPacketOption = 0;

		pNextDynamicAppChannel = nullptr;
		bClosing = FALSE;

		if ( bufData.buff)
			free(bufData.buff);

		bufData.Init();
		ASSERT(pUser && pStaticAppChannel);

		pTransChannel = inpTransChnEx;
		Type = CrossDyncChannel;
		nCount = PushLink(pStaticAppChannel);
	}

	void Initialize(CUserDataEx *inUserEx, ICTEPAppProtocol* inpiAppProt, LPCSTR inAppName,
		CAppChannelEx *inStaAppChn /*= nullptr*/, EmPacketLevel inlevel /*= Middle*/, USHORT inOption /*= 0*/)
	{
		ASSERT(pNext == NULL && pAppParam == nullptr);
		ASSERT(this && inpiAppProt && inStaAppChn != this);
		Init();

		sAppName = inAppName;
		pStaticAppChannel = inStaAppChn;
		pUser = inUserEx;
		dwInterfaceVer = inpiAppProt->GetInterfaceVersion();
		piAppProtocol = inpiAppProt;
		Level = inlevel;
		uPacketOption = inOption;

		pNextDynamicAppChannel = nullptr;
		bClosing = FALSE;

		if ( bufData.buff)
			free(bufData.buff);
		bufData.Init();

		ASSERT(pUser);
		ASSERT( !pStaticAppChannel || 0 == (uPacketOption&Packet_Can_Lost));

		if ( pStaticAppChannel && (uPacketOption&Packet_Can_Lost) && inUserEx->pTransChnUdp)
		{
			pTransChannel = inUserEx->pTransChnUdp;
		}
		else if ( inUserEx->pTransChnTcp)
		{
			pTransChannel = inUserEx->pTransChnTcp;
		}
		else if ( inUserEx->pTransChnMain)
		{
			pTransChannel = inUserEx->pTransChnMain;
		}
		else
		{
			ASSERT(pUser->Status == User_Disconnected);
			pTransChannel = nullptr;
		}

		if ( pStaticAppChannel)	// 当前生成的是一个指定静态通道的附属动态通道
		{
			Type = DynamicChannel;
			StaticACId = pStaticAppChannel->AppChannelId;
			nCount = PushLink(pStaticAppChannel);
		}
		else	// 当前生成的是静态通道
		{
			Type = StaticChannel;
			StaticACId = INVALID_ACID;
			pStaticAppChannel = this;
			nCount = 1;
		}
	}

	inline char* SplitPacket( CTEPPacket_Message* pMsg, __out DWORD& dwSize)
	{
		CTEPPacket_Header *pHeader = (CTEPPacket_Header*)pMsg;
		if ( !pMsg)
		{
			if ( bufData.buff)
			{
				free(bufData.buff);
				bufData.Init();
			}

			return nullptr;
		}

		if ( pHeader->EntirePacket())
		{
			dwSize = pMsg->GetMessageLength();
			return pMsg->GetBuffer();
		}

		DWORD dwEntirSize = pMsg->entireLength;
		if ( dwEntirSize < bufData.size + pMsg->GetMessageLength())// 接收大小超过包大小
		{
			ASSERT(0);
			return (char*)-1;
		}

		if ( pHeader->FirstPacket())
		{
			if ( bufData.buff || bufData.size > 0)
			{
				ASSERT(0);
				return (char*)-1;
			}

			bufData.buff = (char*)malloc(dwEntirSize);
			if ( !bufData.buff)
			{
				ASSERT(0);
				return (char*)-1;
			}

			bufData.maxlength = dwEntirSize;
		}

		memmove_s(bufData.buff + bufData.size, bufData.maxlength - bufData.size, pMsg->GetBuffer(), pMsg->GetMessageLength());
		bufData.size += pMsg->GetMessageLength();

		if ( pHeader->LastPacket())
		{
			if ( dwEntirSize != bufData.size || dwEntirSize != bufData.maxlength)
			{
				ASSERT(0);
				return (char*)-1;
			}

			dwSize = dwEntirSize;
			return bufData.buff;
		}

		return nullptr;
	}

	inline ULONG PushLink(CAppChannelEx* pStaChn)
	{
		ULONG linkId;
		ASSERT(pStaChn && pStaChn == pStaticAppChannel && pStaChn->Type == StaticChannel && Type == DynamicChannel);

		LOCK((CMyCriticalSection*)pStaChn);
		linkId = ++pStaChn->nCount;
		pNextDynamicAppChannel = pStaChn->pNextDynamicAppChannel;
		pStaChn->pNextDynamicAppChannel = this;

		return linkId;
	}

	inline void RemoveLink()// DynamicChannel 从master上移除
	{
		CAppChannelEx *pTemp = pStaticAppChannel;
		if ( pTemp && this != pTemp)
		{
			ASSERT(Type == DynamicChannel);
			LOCK((CMyCriticalSection*)pTemp);
			if ( pStaticAppChannel)
			{
				pStaticAppChannel = nullptr;

				CAppChannelEx *p1, *p2;
				p1 = pTemp;
				p2 = p1->pNextDynamicAppChannel;
				while ( p2 && p2 != this)
				{
					p1 = p1->pNextDynamicAppChannel;
					p2 = p2->pNextDynamicAppChannel;
				}

				if ( p2)
				{
					pTemp->nCount--;
					p1->pNextDynamicAppChannel = p2->pNextDynamicAppChannel;
					p2->pNextDynamicAppChannel = nullptr;
				}
			}
		}
	}
	inline CAppChannelEx* PopLink()// 取出一个动态通道,有可能取出自己,也有可能不是自己
	{
		CAppChannelEx* pResult = nullptr;
		if ( pStaticAppChannel && pStaticAppChannel->pNextDynamicAppChannel)
		{
			LOCK((CMyCriticalSection*)pStaticAppChannel);

			if ( pStaticAppChannel && pStaticAppChannel->pNextDynamicAppChannel)
			{
				pStaticAppChannel->nCount--;
				pResult = pStaticAppChannel->pNextDynamicAppChannel;
				pStaticAppChannel->pNextDynamicAppChannel = pResult->pNextDynamicAppChannel;
			}
			pResult->pNextDynamicAppChannel = nullptr;
		}
		return pResult;
	}

	inline BOOL QueryDisconnect()	// 是否可以关闭此通道
	{
		ASSERT(pUser && this);
		if ( dwInterfaceVer == 1)
		{
			return piAppProtocolEx->QueryDisconnect(pUser, this);
		}

		return TRUE;
	}

public:
	CTransferChannelEx	*pTransChannel;

	CAppChannelEx	*pStaticAppChannel;
	CAppChannelEx	*pNextDynamicAppChannel;

	//保存未完成的一个上层分包
	CBuffer			bufData;

	CAppChannelEx *pNext;
};











