#pragma once

#include <windef.h>
#include <winsock2.h>

#pragma pack(push, 4)
struct CTEPPacket_Header;
struct CTEPPacket_Init;
struct CTEPPacket_Init_Responce;
struct CTEPPacket_Message;

#pragma warning(disable:4482) //warning C4482: 使用了非标准扩展: 限定名中使用了枚举“”

#define CTEP_OPTIONAL		// 可选参数,某些条件下没有

#define CTEP_DEFAULT_BUFFER_SIZE		(4*1024-80)		// I/O请求的缓冲区大小
#define CTEP_DEFAULT_BUFFER_DATA_SIZE	(CTEP_DEFAULT_BUFFER_SIZE - sizeof(CTEPPacket_Message))
#define CTEP_PACKET_HEADER_SIZE			8
#define CTEP_MAX_APPNAME_LENGTH			16	//include tail '\0'

struct CTEPPacket_Header	// sizeof(this) == 8
{
	WORD  PacketLength;		// include this header.

	BYTE  magic;	// 'E'

#define CTEP_PACKET_SEGMENT_MASK		0x03
#define CTEP_PACKET_SEGMENT_FIRST		0x01
#define CTEP_PACKET_SEGMENT_LAST		0x02
#define CTEP_PACKET_SEGMENT_MIDDLE		0x00
#define CTEP_PACKET_SEGMENT_ENTIRE		(CTEP_PACKET_SEGMENT_FIRST|CTEP_PACKET_SEGMENT_LAST)

#define CTEP_PACKET_CONTENT_MASK			0xF0
#define CTEP_PACKET_CONTENT_INIT			(0x10|CTEP_PACKET_SEGMENT_ENTIRE)
#define CTEP_PACKET_CONTENT_INIT_RSP		(0x20|CTEP_PACKET_SEGMENT_ENTIRE)
#define CTEP_PACKET_CONTENT_CREAT_APP		(0x30|CTEP_PACKET_SEGMENT_ENTIRE)
#define CTEP_PACKET_CONTENT_CREAT_APP_RSP	(0x40|CTEP_PACKET_SEGMENT_ENTIRE)
#define CTEP_PACKET_CONTENT_CLOSE_APP		(0x50|CTEP_PACKET_SEGMENT_ENTIRE)
#define CTEP_PACKET_CONTENT_CLOSE_APP_RSP	(0x60|CTEP_PACKET_SEGMENT_ENTIRE)
#define CTEP_PACKET_CONTENT_MESSAGE			(0x70)

	BYTE  Type;

	WORD  UserId;	// User Id (0~65534), (WORD)-1 is invalid.
	WORD  AppChannelId;		// 0~65534, (WORD)-1 is invalid.
	inline bool InvalidUserId()
	{
		return UserId == (WORD)-1;
	}
	inline bool InvalidAppChannelId()
	{
		return AppChannelId == (WORD)-1;
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

	inline bool IsPacketInit()
	{
		return  header.PacketLength == sizeof(CTEPPacket_Init)
			 && header.IsInit();
	}
};
inline int Create_CTEPPacket_Init(CTEPPacket_Init* pBuffer
	, USHORT UserIdAttached = (USHORT)-1, const GUID guid = GUID_NULL)
{
	pBuffer->guidUserSession = guid;
	return Create_CTEPPacket_Header((CTEPPacket_Header*)pBuffer
		, sizeof(CTEPPacket_Init), CTEP_PACKET_CONTENT_INIT, UserIdAttached, (WORD)-1);
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
	memcpy(pBuffer->wsUserName, wsUserName, 260*sizeof(WCHAR));
	if ( ipv4Count>0)
	{
		memcpy(pBuffer->IPv4, ipv4, ipv4Count*sizeof(IN_ADDR));
	}
	return Create_CTEPPacket_Header((CTEPPacket_Header*)pBuffer
								  , (USHORT)(sizeof(CTEPPacket_Init_Responce)+sizeof(IN_ADDR)*ipv4Count)
								  , CTEP_PACKET_CONTENT_INIT_RSP, UserId, (USHORT)-1);
}

struct CTEPPacket_Message
{
	// Type : CTEP_PACKET_CONTENT_MESSAGE | CTEP_PACKET_SEGMENT_???
	CTEPPacket_Header	header;
	CTEP_OPTIONAL DWORD entireLength;	// NeedAssemble() == true
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
	ASSERT(length <= CTEP_DEFAULT_BUFFER_DATA_SIZE);
	ASSERT(dwEntireSize == 0 || dwEntireSize > length);

	

	BYTE Type = CTEP_PACKET_CONTENT_MESSAGE;
	if ( bFirst) Type |= CTEP_PACKET_SEGMENT_FIRST;
	if ( bLast)  Type |= CTEP_PACKET_SEGMENT_LAST;
	
	USHORT dwPacketSize;
	if ( dwEntireSize != 0)
	{
		memcpy(pBuffer->data, data, length);
		pBuffer->entireLength = dwEntireSize;
		dwPacketSize = sizeof(CTEPPacket_Message) + length;
	}
	else
	{
		memcpy((&pBuffer->header + 1), data, length);
		dwPacketSize = sizeof(CTEPPacket_Header) + length;
	}

	return Create_CTEPPacket_Header((CTEPPacket_Header*)pBuffer, dwPacketSize, Type, UserId, AppId);
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
	USHORT				StaticAppChannelId;	// 保存被创建应用所对应静态通道的ID, 如果创建的是静态通道,那么ID为它本身
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







#pragma pack(pop)















