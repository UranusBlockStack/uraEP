#pragma once

#include "CTEP_Communicate_TransferLayer_Interface.h"
#include "CTEP_Communicate_App_Interface.h"

// 这是per-Handle数据。它包含了一个套节字的信息
struct StTransferChannelEx
{
	void InitEx()
	{
		ASSERT( !bNeedNotificationClosing);
		head.Init();
		nOutstandingSend = 0;
		bClosing = TRUE;
		bNeedNotificationClosing = FALSE;
		bufPacket.Init(cPacketBuf);

		pNext = NULL;
	}
	inline HRESULT DisconnectServer(ReadWritePacket* pBuffer = 0)
	{
		return head.piTrans->Disconnect(&head, pBuffer);
	}
	inline HRESULT DisconnectClient()
	{
		return head.piTransClt->Disconnect(&head);
	}
	inline HRESULT SendPacketClient(ReadWritePacket* pBuffer)
	{
		return head.piTransClt->Send(&head, pBuffer);
	}
	inline HRESULT SendPacketServer(ReadWritePacket* pBuffer)
	{
		return head.piTrans->Send(&head, pBuffer);
	}
	
	CTEPPacket_Header* SplitPacket(ReadWritePacket* pPacket)
	{
		long leftsize;
		CTEPPacket_Header* pHeader;

		// 获取pPacket中剩余字节数
		if ( !pPacket->pointer)
		{
			pPacket->pointer = pPacket->buff.buff;
			leftsize = pPacket->buff.size;
		}
		else
		{
			leftsize = pPacket->buff.size - (pPacket->pointer - pPacket->buff.buff);
			ASSERT(leftsize >= 0);
		}

		if ( leftsize <= 0)
			return nullptr;

		// 清除上次拼装好的完整包
		if ( bufPacket.size > 0 && bufPacket.size == bufPacket.maxlength)
			bufPacket.Init(cPacketBuf);

		// 检查是否有上次剩下的字节
		if ( bufPacket.size == 0)
		{
			pHeader = (CTEPPacket_Header*)pPacket->pointer;

			// 检查pPacket中是否是一个完整包
			if (	leftsize < sizeof(CTEPPacket_Header)
				|| pHeader->PacketLength > leftsize)
			{
				memcpy(bufPacket.buff, pPacket->pointer, leftsize);
				pPacket->pointer += leftsize;
				bufPacket.size = leftsize;
				return nullptr;
			}
			else
			{
				pPacket->pointer += pHeader->PacketLength;
				return pHeader;
			}
		}

		// 检查上次剩下的字节是否具有完整的包头
		if ( bufPacket.size < sizeof(CTEPPacket_Header))
		{
			DWORD dwCopy = min((DWORD)leftsize, sizeof(CTEPPacket_Header)-bufPacket.size);
			memcpy(bufPacket.buff+bufPacket.size, pPacket->pointer, dwCopy);
			pPacket->pointer += dwCopy;
			bufPacket.size += dwCopy;

			if ( bufPacket.size < sizeof(CTEPPacket_Header))
				return nullptr;
		}

		pHeader = (CTEPPacket_Header*)bufPacket.buff;
		ASSERT(bufPacket.maxlength == 0 || bufPacket.maxlength == pHeader->PacketLength);
		bufPacket.maxlength = pHeader->PacketLength;
		ASSERT(bufPacket.size <= bufPacket.maxlength
			&& bufPacket.maxlength >= sizeof(CTEPPacket_Header));

		if (	bufPacket.size > bufPacket.maxlength
			|| bufPacket.maxlength < sizeof(CTEPPacket_Header))
		{
			bufPacket.Init(cPacketBuf);
			return (CTEPPacket_Header*)-1;
		}

		if ( bufPacket.size == bufPacket.maxlength)
			return pHeader;

		DWORD dwCopy = min((DWORD)leftsize, bufPacket.maxlength - bufPacket.size);
		memcpy(bufPacket.buff+bufPacket.size, pPacket->pointer, dwCopy);
		pPacket->pointer += dwCopy;
		bufPacket.size += dwCopy;

		if ( bufPacket.size == bufPacket.maxlength)
			return pHeader;

		return nullptr;
	}

public:
	StTransferChannel head;// 

	volatile long nOutstandingSend;// 此套节字上抛出的重叠操作的数量

	volatile BOOL bClosing;					// 套节字是否关闭
	volatile UINT bNeedNotificationClosing;	// 是否需要通知App用户退出, TRUE:需要通知, FALSE:不需要通知

	// 保存未完成的一个底层分包
	CBuffer			bufPacket;
	char			cPacketBuf[CTEP_DEFAULT_BUFFER_SIZE];

	StTransferChannelEx *pNext;
};


class CUserDataEx : public CUserData
	, public CMyCriticalSection
{
public:
	explicit CUserDataEx(StTransferChannelEx* pMain, const GUID& guid = GUID_NULL):pTransChnMain(pMain)
		, pTransChnTcp(nullptr), pTransChnUdp(nullptr), bClosing(FALSE), guidUser(guid)
	{
		UserId = (USHORT)-1;
		dwSessionId = (DWORD)-1;
		ZeroObject(wsUserName);
	}

public:
	BOOL				 bClosing;
	GUID				 guidUser;
	StTransferChannelEx* pTransChnMain;
	StTransferChannelEx* pTransChnTcp;
	StTransferChannelEx* pTransChnUdp;
};

class CAppChannelEx : public CAppChannel, public CMyCriticalSection
{
public:
	CAppChannelEx(CUserDataEx *pUserEx, ICTEPAppProtocol* piAppProt, 
		CAppChannelEx *pStaAppChn /*= nullptr*/, EmPacketLevel level /*= Middle*/, USHORT option /*= 0*/)
		: pStaticAppChannel(pStaAppChn), pNextDynamicAppChannel(nullptr)
	{
		bufData.Init();
		pUser = pUserEx;
		pAppParam = 0;
		AppChannelId = (USHORT)-1;
		piAppProtocol = piAppProt;
		Level = level;
		uPacketOption = option;
		ASSERT(pUser);
		ASSERT( !pStaticAppChannel || 0 == (uPacketOption&Packet_Can_Lost));

		if ( pStaticAppChannel && (uPacketOption&Packet_Can_Lost) && pUserEx->pTransChnUdp)
		{
			this->pTransChannel = (StTransferChannel*)pUserEx->pTransChnUdp;
		}
		else if ( pUserEx->pTransChnTcp)
		{
			this->pTransChannel = (StTransferChannel*)pUserEx->pTransChnTcp;
		}
		else if ( pUserEx->pTransChnMain)
		{
			this->pTransChannel = (StTransferChannel*)pUserEx->pTransChnMain;
		}
		else
		{
			pTransChannel = nullptr;
			ASSERT(0);
		}

		if ( pStaticAppChannel)
		{
			Type = DynamicChannel;
			PushLink(pStaticAppChannel);
		}
		else
		{
			Type = StaticChannel;
			pStaticAppChannel = this;
		}
	}
	~CAppChannelEx()
	{
		if ( bufData.buff)
		{
			free(bufData.buff);
		}
		RemoveLink();
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

		memcpy(bufData.buff + bufData.size, pMsg->GetBuffer(), pMsg->GetMessageLength());
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

	inline void PushLink(CAppChannelEx* pStaChn)
	{
		ASSERT(pStaChn && pStaChn == pStaticAppChannel && Type == DynamicChannel);
		pStaChn->Lock();
		pNextDynamicAppChannel = pStaChn->pNextDynamicAppChannel;
		pStaChn->pNextDynamicAppChannel = this;
		pStaChn->Unlock();
	}
	inline void RemoveLink()// DynamicChannel 从master上移除
	{
		CAppChannelEx *pTemp = pStaticAppChannel;
		if ( pTemp && this != pTemp)
		{
			ASSERT(Type == DynamicChannel);
			pTemp->Lock();
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
					p1->pNextDynamicAppChannel = p2->pNextDynamicAppChannel;
					p2->pNextDynamicAppChannel = nullptr;
				}
			}
			pTemp->Lock();
		}
		else
		{
			ASSERT(Type == StaticChannel);
			ASSERT( pNextDynamicAppChannel == nullptr);	// 静态通道关闭时,必须保证绑定在它上面的所有动态通道都已经退出完毕
		}
	}
	inline CAppChannelEx* PopLink()// 取出一个动态通道,有可能取出自己,也有可能不是自己
	{
		CAppChannelEx* pResult = nullptr;
		if ( pStaticAppChannel && pStaticAppChannel->pNextDynamicAppChannel)
		{
			pStaticAppChannel->Lock();

			if ( pStaticAppChannel && pStaticAppChannel->pNextDynamicAppChannel)
			{
				pResult = pStaticAppChannel->pNextDynamicAppChannel;
				pStaticAppChannel->pNextDynamicAppChannel = pResult->pNextDynamicAppChannel;
			}

			pStaticAppChannel->Unlock();

			pResult->pNextDynamicAppChannel = nullptr;
		}
		return pResult;
	}

public:
	CAppChannelEx	*pStaticAppChannel;
	CAppChannelEx	*pNextDynamicAppChannel;

	//保存未完成的一个上层分包
	CBuffer			bufData;
};











