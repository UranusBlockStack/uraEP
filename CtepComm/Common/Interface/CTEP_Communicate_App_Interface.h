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
	InvalidChannel = -1,
	StaticChannel = 0,
	DynamicChannel = 1,
	CrossDyncChannel = 2,
};

class __declspec(novtable) CAppChannel		// 一条应用通道, 表示一个应用对一个用户的服务
{
public:
	void Init()
	{
		bClosing = TRUE;

		pAppParam = NULL;
		Type = InvalidChannel;
		AppChannelId = INVALID_ACID;
		StaticACId   = INVALID_ACID;
		TargetACId   = INVALID_ACID;

		dwInterfaceVer = (DWORD)-1;
		piAppProtocol = nullptr;
		pUser = nullptr;

		uPacketOption = 0;
		Level = Low;
		nCount = 0;
		sAppName = nullptr;
	}

public:
	READONLY_DATA  BOOL				bClosing;
	READWRITE_DATA void*			pAppParam;

	READONLY_DATA  EnAppChannelType	Type;			//通道类型,静态通道或者动态通道

	READONLY_DATA  USHORT			AppChannelId;
	READONLY_DATA  USHORT			StaticACId;		// 动态通道所属的静态通道,如果是静态通道此值为INVALID_ACID

	READONLY_DATA  DWORD			dwInterfaceVer;
	READWRITE_DATA USHORT			TargetACId;
	union
	{
		READONLY_DATA ICTEPAppProtocol*   piAppProtocol;		// 通道对应的服务接口
		READONLY_DATA ICTEPAppProtocolEx* piAppProtocolEx;		// 通道对应的服务接口, 当dwInterfaceVer = 1时有效
	};
	
	READONLY_DATA CUserData			*pUser;			// 通道对应的用户

#define Packet_Can_Lost				0x0001
#define Packet_Need_Encryption		0x1000
	READONLY_DATA USHORT			uPacketOption;
	READONLY_DATA EmPacketLevel		Level;

	READONLY_DATA ULONG				nCount;			// App Dynamic Channel Count

	READONLY_DATA LPCSTR			sAppName;		// 应用名称

public:
	inline LPCWSTR debugType()
	{
		switch(Type)
		{
		case StaticChannel:
			return L"StaticChannel";
		case DynamicChannel:
			return L"DynamicChannel";
		case CrossDyncChannel:
			return L"CrossDyncChannel";
		}
		return L"EnAppChannelType:??????";
	}
};

interface ICTEPAppProtocolCallBack
{
	virtual HRESULT WritePacket(USHORT AppChannelId, char* buff, ULONG size, CUserData* pUser = nullptr) = 0;
	virtual HRESULT WritePacket(CAppChannel *, char* buff, ULONG size) = 0;
	virtual HRESULT WritePacket(CAppChannel *, ReadWritePacket *) = 0;

	virtual CAppChannel* CreateDynamicChannel(CAppChannel* pStaticChannel
		 , EmPacketLevel Level = Middle, USHORT dwType = NULL) = 0;
	virtual void		 CloseDynamicChannel(CAppChannel* pDynamicChannel) = 0;
	virtual HRESULT		 CloseDynamicChannel(USHORT AppChannelId, CUserData* pUser = nullptr) = 0;

	virtual CAppChannel* LockChannel(USHORT AppChannelId, CUserData* pUser = nullptr) = 0;
	virtual void		 UnlockChannel(CAppChannel* pAppChannel) = 0;

	virtual ReadWritePacket* MallocSendPacket(CAppChannel *pAppChn, USHORT size) = 0;// size > 0 && size < CTEP_DEFAULT_BUFFER_DATA_SIZE
	virtual void			 FreePacket(ReadWritePacket *) = 0;

	// CTEP 2.0
	virtual HRESULT RegisterCallBackEvent(StCallEvent Calls[], DWORD dwCallCount) = 0;
	virtual HRESULT UnregisterCallBackEvent(StCallEvent Calls[], DWORD dwCallCount) = 0;
};

interface ICTEPAppProtocol
{
	virtual LPCSTR   GetName() = 0;	// AppName character count <= 15, 
	virtual DWORD    GetInterfaceVersion() = 0; //默认返回0, ICTEPAppProtocolEx接口返回1,

#define CTEP_TYPE_APP_SERVER	1
#define CTEP_TYPE_APP_CLIENT	0
	virtual HRESULT  Initialize(ICTEPAppProtocolCallBack* pI, DWORD dwType) = 0;
	virtual void	 Final() = 0;	// server close.

	virtual HRESULT  Connect(CUserData* pUser, CAppChannel* pNewAppChannel, CAppChannel* pStaticAppChannel = nullptr) = 0;	// 一个被动态通道
	virtual HRESULT  ReadPacket(CUserData* pUser, CAppChannel *pAppChannel, char* pBuff, ULONG size) = 0;
	virtual void	 Disconnect(CUserData* pUser, CAppChannel *pAppChannel) = 0;													// 一个动态通道被
};

interface ICTEPAppProtocolEx : public ICTEPAppProtocol
{
	virtual DWORD    GetNameCount() = 0;// 支持的App数量,默认为1, Proxy应用有可能会代理多个App.
	virtual LPCSTR   GetNameIndex(long iIndex = 0) = 0;	// AppName character count <= 15, 用于Proxy支持多个App的代理使用

	virtual BOOL     QueryDisconnect(CUserData* pUser, CAppChannel *pAppChannel) = 0;	//Comm模块询问App模块指定静态通道是否可以被关闭了,可以返回TRUE,拒绝关闭返回FALSE

	virtual void     ChannelStateChanged(CUserData* pUser, CAppChannel *pAppChannel) = 0;	//Comm模块通知App模块指定通道的状态发生改变(Connected, Disconnected)

	virtual BOOL     ConnectCrossApp(CUserData* pUser, CAppChannel* pNewAppChannel, CAppChannel* pStaticAppChannel) = 0;
};

// 
// 用户跨进程的CTEP App通讯使用的接口.
// 
enum EmCtepAppProxyTargetChannel
{
	StaticAppChannel = 0,	//发往 or 接收 同侧的静态通道数据
	BroadCastAppChannel,	//发往 or 接收 同侧的所有通道广播数据
	SpecialAppChannel,		//发往 or 接收 同侧的指定的某个通道数据
	RemoteAppChannel,		//发往 or 接收 另一侧(server2client, client2server)的通道数据
};

interface ICTEPAppProtocolCrossCallBack
{
	virtual HRESULT Connect(LPCSTR sAppName, CUserData* pUser, EnAppChannelType type) = 0;//创建一个当前用户当前应用的通道(动态或者静态)
	virtual HRESULT Disconnect();

	virtual HRESULT WritePacket(EmCtepAppProxyTargetChannel type, USHORT targetAppId
		, char* buff, ULONG size) = 0; //向指定AppChannel通道发送数据, 只能发往当前用户的这个应用的App通道之间互相发送.

#define CTEP_RESULT_S_OK				0
#define CTEP_RESULT_S_TIMEOUT			5
#define CTEP_RESULT_E_FAILD				-1

	virtual HRESULT ReadPacket(__out char** pBuff, __out DWORD *pSize, __out EmCtepAppProxyTargetChannel *pType, __out USHORT *pAppId);
};


