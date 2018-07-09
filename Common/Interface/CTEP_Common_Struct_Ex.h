#pragma once

#include "Log4Cpp_Lib/Log4CppLib.h"
#include "CommonInclude/DynamicBuffer.h"

#include "CTEP_Communicate_TransferLayer_Interface.h"
#include "CTEP_Communicate_App_Interface.h"

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
		
		dwPacketSize = rbufPacket.PushData(pPacket->buff.size);
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
		dwPacketSize = rbufPacket.PushDataAndPop(pPacket->buff.size, (void**)&pHeader);
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

		if ( SequenceIdRemote == 0)
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
	explicit CUserDataEx(CTransferChannelEx* pMain, const GUID& guid = GUID_NULL):pTransChnMain(pMain)
		, pTransChnTcp(nullptr), pTransChnUdp(nullptr), bClosing(TRUE), guidUser(guid)
	{
		UserId = (USHORT)-1;
		dwSessionId = (DWORD)-1;
		ZeroObject(wsUserName);
	}

public:
	BOOL				 bClosing;
	GUID				 guidUser;
	CTransferChannelEx* pTransChnMain;
	CTransferChannelEx* pTransChnTcp;
	CTransferChannelEx* pTransChnUdp;
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

		RemoveLink();

		sAppName = nullptr;
		pStaticAppChannel = pNextDynamicAppChannel = nullptr;
		pUser = nullptr;
		ASSERT(pAppParam == nullptr);
		AppChannelId = (USHORT)-1;
		piAppProtocol = nullptr;
		Level = Low;
		uPacketOption = 0;
		pTransChannel = nullptr;
		nCount = 0;
	}

	void Initialize(CUserDataEx *inUserEx, ICTEPAppProtocol* inpiAppProt, LPCSTR inAppName,
		CAppChannelEx *inStaAppChn /*= nullptr*/, EmPacketLevel inlevel /*= Middle*/, USHORT inOption /*= 0*/)
	{
		pNext = nullptr;
		sAppName = inAppName;
		pStaticAppChannel = inStaAppChn;
		pNextDynamicAppChannel = nullptr;

		bClosing = FALSE;

		if ( bufData.buff)
		{
			free(bufData.buff);
		}
		bufData.Init();
		pUser = inUserEx;
		ASSERT(pAppParam == nullptr);
		pAppParam = nullptr;
		AppChannelId = (USHORT)-1;
		piAppProtocol = inpiAppProt;
		Level = inlevel;
		uPacketOption = inOption;
		ASSERT(pUser);
		ASSERT( !pStaticAppChannel || 0 == (uPacketOption&Packet_Can_Lost));

		if ( pStaticAppChannel && (uPacketOption&Packet_Can_Lost) && inUserEx->pTransChnUdp)
		{
			this->pTransChannel = inUserEx->pTransChnUdp;
		}
		else if ( inUserEx->pTransChnTcp)
		{
			this->pTransChannel = inUserEx->pTransChnTcp;
		}
		else if ( inUserEx->pTransChnMain)
		{
			this->pTransChannel = inUserEx->pTransChnMain;
		}
		else
		{
			pTransChannel = nullptr;
			ASSERT(0);
		}

		if ( pStaticAppChannel)
		{
			Type = DynamicChannel;
			nCount = PushLink(pStaticAppChannel);
		}
		else
		{
			Type = StaticChannel;
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

public:
	CTransferChannelEx	*pTransChannel;

	CAppChannelEx	*pStaticAppChannel;
	CAppChannelEx	*pNextDynamicAppChannel;

	//保存未完成的一个上层分包
	CBuffer			bufData;

	CAppChannelEx *pNext;
};











