#pragma once

#include <windef.h>
#include <tchar.h>
#include "CTEP_Trans_Packet_Protocol.h"

#define READWRITE_DATA
#define READONLY_DATA
#define OPAQUE_DATA

class  CUserData;
class  CAppChannel;
class  CTransferChannel;
struct ReadWritePacket;


interface ICTEPTransferProtocolCallBack;
interface ICTEPTransferProtocolServer;
interface ICTEPTransferProtocolClient;

interface ICTEPAppProtocolCallBack;
interface ICTEPAppProtocol;

#define FUNC_CTEPGetInterfaceTransServer	"CTEPGetInterfaceTransServer"
typedef ICTEPTransferProtocolServer* (WINAPI *Fn_CTEPGetInterfaceTransServer)();
ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServer();

#define FUNC_CTEPGetInterfaceTransClient	"CTEPGetInterfaceTransClient"
typedef ICTEPTransferProtocolClient* (WINAPI *Fn_CTEPGetInterfaceTransClient)();
ICTEPTransferProtocolClient* WINAPI CTEPGetInterfaceTransClient();

#define FUNC_CTEPGetInterfaceAppServer		"CTEPGetInterfaceAppServer"
typedef ICTEPAppProtocol* (WINAPI *Fn_CTEPGetInterfaceAppServer)();
ICTEPAppProtocol* WINAPI CTEPGetInterfaceAppServer();

#define FUNC_CTEPGetInterfaceAppClient		"CTEPGetInterfaceAppClient"
typedef ICTEPAppProtocol* (WINAPI *Fn_CTEPGetInterfaceAppClient)();
ICTEPAppProtocol* WINAPI CTEPGetInterfaceAppClient();


class ATL_NO_VTABLE CUserData
{
public:
	USHORT UserId;
	DWORD dwSessionId;
	WCHAR wsUserName[260];
};

enum EmPacketOperationType
{
	OP_Empty = 0,
	OP_Listen,
	OP_IocpSend,
	OP_IocpRecv,
	OP_PacketRecv,
};

struct CBuffer
{
	void Init(char* buffer = 0, DWORD buffsize = 0)
	{
		buff = buffer;
		size = 0;
		maxlength = buffsize;
	}
	char* buff;
	DWORD size;
	DWORD maxlength;
};

struct ReadWritePacket
{
	void PacketInit()
	{
		buff.Init(cBuf, CTEP_DEFAULT_BUFFER_SIZE);
		pointer = nullptr;
		pTransChn = NULL;
		opType = OP_Empty;
		piTrans = NULL;
		hFile = INVALID_HANDLE_VALUE;
		pNext = nullptr;
		ol.Internal = 0;
		ol.InternalHigh = 0;
	}

	union
	{
		WSAOVERLAPPED wsaol;
		OVERLAPPED	  ol;
	};

	READWRITE_DATA EmPacketOperationType opType;

	union
	{
		READWRITE_DATA HANDLE  hFile;
		READWRITE_DATA SOCKET  s;
	};

	OPAQUE_DATA CBuffer			buff;			// default: pBuffer = Buf;
	OPAQUE_DATA CHAR* volatile	pointer;

	union
	{
		READWRITE_DATA ICTEPTransferProtocolServer*	piTrans;
		READWRITE_DATA ICTEPTransferProtocolClient*	piTransClt;
	};
	
	OPAQUE_DATA CTransferChannel * pTransChn;

	OPAQUE_DATA ReadWritePacket  * pNext;

	OPAQUE_DATA CHAR	 cBuf[CTEP_DEFAULT_BUFFER_SIZE];
};







