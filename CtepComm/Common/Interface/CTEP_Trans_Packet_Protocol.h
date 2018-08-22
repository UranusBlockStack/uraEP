#pragma once

#include <windef.h>
#include <winsock2.h>

#pragma pack(push, 1)
struct CTEPPacket_Header;
struct CTEPPacket_Init;
struct CTEPPacket_Init_Responce;
struct CTEPPacket_Message;

#pragma warning(disable:4482) //warning C4482: 使用了非标准扩展: 限定名中使用了枚举“”

//#define CTEP_OPTIONAL		// 可选参数,某些条件下没有

#define INVALID_ACID					((USHORT)-1)	// 无效的AppChannelId数值,包括StaticChannel, DynamicChannel, CrossAppChannel
#define INVALID_UID						((USHORT)-1)	// 无效的UserId数值,包括Session相关User与Session无关User.
#define INVALID_SESSIONID				((DWORD)-1)		// 无效的windows Session Id.

#define CTEP_DEFAULT_BUFFER_SIZE		(4096*4-80)		// I/O请求的缓冲区大小, RDP通道在较大大小时会丢失数据
#define CTEP_DEFAULT_BUFFER_DATA_SIZE	(CTEP_DEFAULT_BUFFER_SIZE - sizeof(CTEPPacket_Message))
#define CTEP_PACKET_HEADER_SIZE			8
#define CTEP_MAX_APPNAME_LENGTH			16	//include tail '\0'

struct CTEPPacket_Header	// sizeof(this) == 8 + 4
{
// Data Define
	WORD  PacketLength;		// include this header.

	BYTE  magic;	// 'E'

#define CTEP_PACKET_SEGMENT_MASK					0x03
#define CTEP_PACKET_SEGMENT_FIRST					0x01
#define CTEP_PACKET_SEGMENT_LAST					0x02
#define CTEP_PACKET_SEGMENT_MIDDLE					0x00
#define CTEP_PACKET_SEGMENT_ENTIRE					(CTEP_PACKET_SEGMENT_FIRST|CTEP_PACKET_SEGMENT_LAST)

#define CTEP_PACKET_CONTENT_MASK					0xF0
#define CTEP_PACKET_CONTENT_HELLO					(0xB0|CTEP_PACKET_SEGMENT_ENTIRE)
#define CTEP_PACKET_CONTENT_HELLO_RSP				(0xC0|CTEP_PACKET_SEGMENT_ENTIRE)
#define CTEP_PACKET_CONTENT_INIT					(0x10|CTEP_PACKET_SEGMENT_ENTIRE)
#define CTEP_PACKET_CONTENT_INIT_RSP				(0x20|CTEP_PACKET_SEGMENT_ENTIRE)
#define CTEP_PACKET_CONTENT_CREAT_APP				(0x30|CTEP_PACKET_SEGMENT_ENTIRE)
#define CTEP_PACKET_CONTENT_CREAT_APP_RSP			(0x40|CTEP_PACKET_SEGMENT_ENTIRE)
#define CTEP_PACKET_CONTENT_CLOSE_APP				(0x50|CTEP_PACKET_SEGMENT_ENTIRE)
#define CTEP_PACKET_CONTENT_CLOSE_APP_RSP			(0x60|CTEP_PACKET_SEGMENT_ENTIRE)
#define CTEP_PACKET_CONTENT_MESSAGE					(0x70)
#define CTEP_PACKET_CONTENT_MSG_LOCAL_STATIC		(0x80|CTEP_PACKET_SEGMENT_ENTIRE)
#define CTEP_PACKET_CONTENT_MSG_LOCAL_BROADCAST		(0x90|CTEP_PACKET_SEGMENT_ENTIRE)
#define CTEP_PACKET_CONTENT_MSG_LOCAL_SPECIAL		(0xA0|CTEP_PACKET_SEGMENT_ENTIRE)

#define CTEP_PACKET_CONTENT_INITCROSSAPP			CTEP_PACKET_CONTENT_INIT			//CTEP v2.0
#define CTEP_PACKET_CONTENT_INITCROSSAPP_RSP		CTEP_PACKET_CONTENT_INIT_RSP		//CTEP v2.0
	BYTE  Type;

	WORD  UserId;			// User Id (0~65534), INVALID_UID is invalid.
	WORD  AppChannelId;		// 0~65534, INVALID_ACID is invalid.

	DWORD SequenceId;

// Debug
	inline LPWSTR debugType()
	{
		if ( IsCloseApp())
			return L"IsCloseApp";
		else if ( IsCloseAppRsp())
			return L"IsCloseAppRsp";
		else if ( IsCreateApp())
			return L"IsCreateApp";
		else if ( IsCreateAppRsp())
			return L"IsCreateAppRsp";
		else if ( IsHello())
			return L"IsHello";
		else if ( IsHelloRsp())
			return L"IsHelloRsp";
		else if ( IsInit())
			return L"IsInit";
		else if ( IsInitRsp())
			return L"IsInitRsp";
		else if ( IsMessage())
			return L"IsMessage";

		return L"Is??????";
	}

// Function Define
	inline bool InvalidUserId()
	{
		return UserId == INVALID_UID;
	}
	inline bool InvalidAppChannelId()
	{
		return AppChannelId == INVALID_ACID;
	}

	inline bool EntirePacket()
	{
		return (Type & CTEP_PACKET_SEGMENT_MASK) == CTEP_PACKET_SEGMENT_ENTIRE;
	}
	inline bool FirstPacket()
	{
		return (Type & CTEP_PACKET_SEGMENT_MASK) == CTEP_PACKET_SEGMENT_FIRST;
	}
	inline bool LastPacket()
	{
		return (Type & CTEP_PACKET_SEGMENT_MASK) == CTEP_PACKET_SEGMENT_LAST;
	}

	inline bool IsHello()
	{
		return magic == 'E' && Type == CTEP_PACKET_CONTENT_HELLO;
	}
	inline bool IsHelloRsp()
	{
		return magic == 'E' && Type == CTEP_PACKET_CONTENT_HELLO_RSP;
	}
	inline bool IsInit()
	{
		return magic == 'E' && Type == CTEP_PACKET_CONTENT_INIT;
	}
	inline bool IsInitRsp()
	{
		return magic == 'E' && Type == CTEP_PACKET_CONTENT_INIT_RSP;
	}
	inline bool IsCreateApp()
	{
		return magic == 'E' && Type == CTEP_PACKET_CONTENT_CREAT_APP;
	}
	inline bool IsCreateAppRsp()
	{
		return magic == 'E' && Type == CTEP_PACKET_CONTENT_CREAT_APP_RSP;
	}
	inline bool IsCloseApp()
	{
		return magic == 'E' && Type == CTEP_PACKET_CONTENT_CLOSE_APP;
	}
	inline bool IsCloseAppRsp()
	{
		return magic == 'E' && Type == CTEP_PACKET_CONTENT_CLOSE_APP_RSP;
	}
	inline bool IsMessage()
	{
		return magic == 'E' &&
			(Type & CTEP_PACKET_CONTENT_MASK) == CTEP_PACKET_CONTENT_MESSAGE;
	}
	inline DWORD GetContentLength()
	{
		if ( PacketLength <= sizeof(CTEPPacket_Header))
			return 0;

		return PacketLength - sizeof(CTEPPacket_Header);
	}
	inline WORD GetUserId()
	{
		return UserId;
	}
	inline WORD GetAppId()
	{
		return AppChannelId;
	}
};
inline int Create_CTEPPacket_Header(CTEPPacket_Header* pBuffer, USHORT PacketLength, BYTE type
	, WORD UserId, WORD AppId)
{
	pBuffer->PacketLength = PacketLength;
	pBuffer->magic = 'E';
	pBuffer->UserId = UserId;
	pBuffer->AppChannelId = AppId;
	pBuffer->Type = type;
	return PacketLength;
}

// this packet is handshake
struct CTEPPacket_Hello
{
	// PacketLength = sizeof(CTEPPacket_Hello)
	// Type : CTEP_PACKET_CONTENT_HELLO / CTEP_PACKET_CONTENT_HELLO_RSP
	CTEPPacket_Header	header;
	WCHAR				msg[32];
};
inline int Create_CTEPPacket_Hello(CTEPPacket_Hello* pBuffer, LPCWSTR msg, BOOL bResponse = TRUE)
{
	BYTE type = CTEP_PACKET_CONTENT_HELLO;
	if ( bResponse)
		type = CTEP_PACKET_CONTENT_HELLO_RSP;

	wcscpy_s(pBuffer->msg, 31, msg);
	pBuffer->msg[31] = 0;
	return Create_CTEPPacket_Header((CTEPPacket_Header*)pBuffer
		, sizeof(CTEPPacket_Hello), type, INVALID_UID, INVALID_ACID);
}


// this packet is send from CTEP-Client to CTEP-Server
struct CTEPPacket_Init
{
	// PacketLength = sizeof(CTEPPacket_Init)
	// UserSession assist with is: -1 (with master transfer layer channel)
	//							or 0~65534 (with assist transfer layer channel)
	// Type : CTEP_PACKET_CONTENT_INIT
	CTEPPacket_Header	header;

	// GUID_Emply(CTEP_PACKET_CONTENT_INIT_MASTER) or Server User Session Guid.
	GUID				guidUserSession;
	WCHAR				wsUserName[260];

	inline bool IsPacketInit()
	{
		return  header.PacketLength == sizeof(CTEPPacket_Init)
			 && header.IsInit();
	}
};
inline int Create_CTEPPacket_Init(CTEPPacket_Init* pBuffer
	, USHORT UserIdAttached = INVALID_UID, const GUID& guid = GUID_NULL
	, WCHAR UserName[260] = nullptr)
{
	pBuffer->guidUserSession = guid;
	if ( UserName)
	{
		wcscpy_s(pBuffer->wsUserName, UserName);
	}
	else
	{
		pBuffer->wsUserName[0] = NULL;
	}
	return Create_CTEPPacket_Header((CTEPPacket_Header*)pBuffer
		, sizeof(CTEPPacket_Init), CTEP_PACKET_CONTENT_INIT, UserIdAttached, INVALID_ACID);
}

struct CTEPPacket_Init_Responce
{
	// PacketLength = sizeof(CTEPPacket_Init_Responce)
	// UserSession assist with is: 0~65534
	// Type : CTEP_PACKET_CONTENT_INIT_RSP
	CTEPPacket_Header	header;
	
	// Server User Session Guid.
	GUID				guidUserSession;
	DWORD				dwSessionId;
	WCHAR				wsUserName[260];

	USHORT				TcpPort;
	USHORT				UdpPort;

	DWORD				IPv4Count;
#pragma warning(suppress:4200)	//warning C4200: 使用了非标准扩展 : 结构/联合中的零大小数组
	IN_ADDR				IPv4[0];

	inline bool IsPacketInitRsp()
	{
		return  header.PacketLength == (sizeof(CTEPPacket_Init_Responce) + sizeof(IPv4[0])*IPv4Count)
			 && header.IsInitRsp();
	}
};
inline int Create_CTEPPacket_Init_Responce(CTEPPacket_Init_Responce* pBuffer, USHORT UserId
	, DWORD dwSessionId, const GUID& guidUser, WCHAR wsUserName[260]
	, USHORT TcpPort, USHORT UdpPort, IN_ADDR ipv4[], DWORD ipv4Count)
{
	pBuffer->guidUserSession = guidUser;
	pBuffer->dwSessionId = dwSessionId;
	pBuffer->TcpPort = TcpPort;
	pBuffer->UdpPort = UdpPort;
	pBuffer->IPv4Count = ipv4Count;
		wcscpy_s(pBuffer->wsUserName, 260, wsUserName);
	if ( ipv4Count>0)
	{
		memcpy(pBuffer->IPv4, ipv4, ipv4Count*sizeof(IN_ADDR));
	}
	return Create_CTEPPacket_Header((CTEPPacket_Header*)pBuffer
								  , (USHORT)(sizeof(CTEPPacket_Init_Responce)+sizeof(IN_ADDR)*ipv4Count)
								  , CTEP_PACKET_CONTENT_INIT_RSP, UserId, INVALID_ACID);
}

struct CTEPPacket_Message
{
	// Type : CTEP_PACKET_CONTENT_MESSAGE | CTEP_PACKET_SEGMENT_???
	CTEPPacket_Header	header;
	DWORD entireLength;	// NeedAssemble() == true
#pragma warning(suppress:4200)	//warning C4200: 使用了非标准扩展 : 结构/联合中的零大小数组
	CHAR				data[0];

	inline bool IsPacketMessage()
	{
		if ( !header.IsMessage())
			return false;

		if ( GetMessageLength() == 0)
			return false;

		return true;
	}

	inline DWORD GetMessageLength()
	{
		if ( header.EntirePacket())
		{
			return header.GetContentLength();
		}
		else
		{
			if ( header.PacketLength > sizeof(CTEPPacket_Message))
			{
				return header.PacketLength - sizeof(CTEPPacket_Message);
			}
		}

		return 0;
	}

	inline char* GetBuffer()
	{
		if ( header.EntirePacket())
		{
			return (char*)((&header)+1);
		}

		return (char*)data;
	}
};

inline int Create_CTEPPacket_Message(CTEPPacket_Message* pBuffer, USHORT UserId, USHORT AppId
	, char* data, USHORT length, DWORD dwEntireSize = 0
	, bool bFirst = true, bool bLast = true)
{
	ASSERT(length <= CTEP_DEFAULT_BUFFER_DATA_SIZE && length > 0);
	ASSERT(dwEntireSize == 0 || dwEntireSize > length);

	BYTE Type = CTEP_PACKET_CONTENT_MESSAGE;
	if ( bFirst) Type |= CTEP_PACKET_SEGMENT_FIRST;
	if ( bLast)  Type |= CTEP_PACKET_SEGMENT_LAST;
	
	USHORT dwPacketSize;
	if ( dwEntireSize != 0)
	{
		pBuffer->entireLength = dwEntireSize;
		dwPacketSize = sizeof(CTEPPacket_Message) + length;
	}
	else
	{
		dwPacketSize = sizeof(CTEPPacket_Header) + length;
	}

	int iResult = Create_CTEPPacket_Header((CTEPPacket_Header*)pBuffer, dwPacketSize, Type, UserId, AppId);
	if ( data)
	{
		memmove(pBuffer->GetBuffer(), data, length);
	}

	return iResult;
}

struct CTEPPacket_CreateApp
{
	CTEPPacket_Header	header;		// 创建静态通道时AppChannelId = -1, 创建动态通道时AppChannelId为对应静态通道的通道ID
	ULONG64				Key;
	USHORT				uPacketOption;
	USHORT				ePacketLevel;
	char				AppName[16];
};
inline int Create_CTEPPacket_CreateApp(CTEPPacket_CreateApp *pBuffer, USHORT UserId, USHORT AppId
	,  ULONG64 Key, const char AppName[16]
	, USHORT uPacketOption, USHORT ePacketLevel)
{
	pBuffer->Key = Key;
	pBuffer->uPacketOption = uPacketOption;
	pBuffer->ePacketLevel = ePacketLevel;
	strcpy_s(pBuffer->AppName, AppName);
	return Create_CTEPPacket_Header((CTEPPacket_Header*)pBuffer, sizeof(CTEPPacket_CreateApp)
		, CTEP_PACKET_CONTENT_CREAT_APP, UserId, AppId);
}
struct CTEPPacket_CreateAppRsp
{
	CTEPPacket_Header	header;		//AppChannelId内保存被创建的应用通道Id
	ULONG64				Key;
	USHORT				uPacketOption;
	USHORT				ePacketLevel;
	char				AppName[16];
	USHORT				StaticAppChannelId;	// 保存被创建应用所对应静态通道的ID, 如果创建的是静态通道,那么ID为它本身 or INVALID_ACID
	USHORT				bResult;	// 1:TRUE, 0:FALSE
};
inline int Create_CTEPPacket_CreateAppRsp(CTEPPacket_CreateAppRsp *pBuffer, USHORT UserId, USHORT AppId
	, ULONG64 Key, const char AppName[16]
	, USHORT uPacketOption, USHORT ePacketLevel
	, USHORT StaticAppChannelId, BOOL bResult = TRUE)
{
	pBuffer->Key = Key;
	pBuffer->uPacketOption = uPacketOption;
	pBuffer->ePacketLevel = ePacketLevel;
	strcpy_s(pBuffer->AppName, AppName);
	pBuffer->StaticAppChannelId = StaticAppChannelId;
	pBuffer->bResult = (USHORT)bResult;
	return Create_CTEPPacket_Header((CTEPPacket_Header*)pBuffer, sizeof(CTEPPacket_CreateAppRsp)
		, CTEP_PACKET_CONTENT_CREAT_APP_RSP, UserId, AppId);
}

struct CTEPPacket_CloseApp
{
	CTEPPacket_Header	header;
};

struct CTEPPacket_CloseAppRsp
{
	CTEPPacket_Header	header;
};


// 
// CTEP 2.0 new type:
// 

// this packet is send from CTEP-Server-Out-Process-App to CTEP-Server
struct CTEPPacket_InitCrossApp
{
	// PacketLength = sizeof(CTEPPacket_InitCrossApp)
	// Type : CTEP_PACKET_CONTENT_INITCROSSAPP
	CTEPPacket_Header	header;
	DWORD				dwSessionId;
	CHAR				sAppName[16];

	inline bool IsPacketInitCrossApp()
	{
		return  header.PacketLength == sizeof(CTEPPacket_InitCrossApp)
			&& header.IsInit();
	}
};
inline int Create_CTEPPacket_InitCrossApp(CTEPPacket_InitCrossApp* pPacket
	, CHAR AppName[16], DWORD SessionId)
{
	pPacket->dwSessionId = SessionId;
	strcpy_s(pPacket->sAppName, AppName);
	return Create_CTEPPacket_Header((CTEPPacket_Header*)pPacket
		, sizeof(CTEPPacket_InitCrossApp), CTEP_PACKET_CONTENT_INIT, INVALID_UID, INVALID_ACID);
}

// this packet is send from CTEP-Server to CTEP-Server-Out-Process-App
struct CTEPPacket_InitCrossApp_Responce
{
	// PacketLength = sizeof(CTEPPacket_InitCrossApp_Responce)
	// UserId: 0~65534
	// AppChannelId: 0~65534
	// Type : CTEP_PACKET_CONTENT_INIT_RSP
	CTEPPacket_Header	header;

	// Server User Guid.
	GUID				guidUserSession;

	inline bool IsPacketInitCrossAppRsp()
	{
		return  header.PacketLength == sizeof(CTEPPacket_InitCrossApp_Responce)
			 && header.IsInitRsp();
	}
};

inline int Create_CTEPPacket_InitCrossApp_Responce(CTEPPacket_InitCrossApp_Responce* pBuffer
								, USHORT AppChannelId, USHORT UserId, const GUID& guidUser)
{
	pBuffer->guidUserSession = guidUser;
	return Create_CTEPPacket_Header((CTEPPacket_Header*)pBuffer
		, (USHORT)sizeof(CTEPPacket_InitCrossApp_Responce)
		, CTEP_PACKET_CONTENT_INIT_RSP, UserId, AppChannelId);
}




#pragma pack(pop)















