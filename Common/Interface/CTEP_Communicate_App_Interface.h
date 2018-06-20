#pragma once

#include "CTEP_Common_struct.h"

enum EmPacketLevel
{
	Low = 0,
	Middle = 1,
	High = 2,
};

enum EnAppChannelType
{
	StaticChannel = 0,
	DynamicChannel = 1,
};

class CAppChannel		// 一条应用通道, 表示一个应用对一个用户的服务
{
public:
	READWRITE_DATA void*			pAppParam;

	READONLY_DATA EnAppChannelType	Type;			//通道类型,静态通道或者动态通道

	READONLY_DATA USHORT			AppChannelId;
	READONLY_DATA ICTEPAppProtocol* piAppProtocol;	// 通道对应的服务接口
	READONLY_DATA CUserData			*pUser;			// 通道对应的用户

#define Packet_Can_Lost				0x0001
#define Packet_Need_Encryption		0x1000
	READONLY_DATA USHORT			uPacketOption;
	READONLY_DATA EmPacketLevel		Level;

	OPAQUE_DATA StTransferChannel	*pTransChannel;
};


interface ICTEPAppProtocolCallBack
{
	virtual HRESULT WritePacket(CAppChannel *, char* buff, ULONG size) = 0;
	virtual HRESULT WritePacket(CAppChannel *, ReadWritePacket *) = 0;

	virtual CAppChannel* CreateDynamicChannel(CAppChannel* pStaticChannel
		 , EmPacketLevel Level = Middle, USHORT dwType = NULL) = 0;
	virtual void	CloseDynamicChannel(CAppChannel* pDynamicChannel) = 0;

	virtual CAppChannel* LockChannel(USHORT AppChannelId) = 0;
	virtual void UnlockChannel(CAppChannel* pAppChannel) = 0;

	virtual ReadWritePacket*	MallocPacket(CAppChannel *pAppChn, ULONG size = 0) = 0;
	virtual void				FreePacket(ReadWritePacket *) = 0;
};


interface ICTEPAppProtocol
{
	virtual LPCSTR   GetName() = 0;	// AppName character count <= 15
	virtual HRESULT  Initialize(ICTEPAppProtocolCallBack* pI) = 0;
	virtual void	 Final() = 0;	// server close.

	virtual HRESULT  Connect(CUserData* pUser, CAppChannel* pNewAppChannel, CAppChannel *pStaticAppChannel/* = nullptr*/) = 0;
	virtual HRESULT  ReadPacket(CUserData* pUser, CAppChannel *pAppStaticChannel, char* pBuff, ULONG size) = 0;
	virtual void	 Disconnect(CUserData* pUser, CAppChannel *pAppStaticChannel) = 0;
};






