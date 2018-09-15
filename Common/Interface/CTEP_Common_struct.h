// 本文档描述了文件结构层面的文件名称与导出函数名.
// 设计的有些复杂,不过我已经尽量简化了,功能很复杂,能设计成这样已经不错了!如果看不懂请多看几遍.
// 如果还是看不懂,那一定是你智商的问题,请不要怀疑设计有缺陷,谢谢合作!

#pragma once

#include <windef.h>
#include <tchar.h>
#include <ws2ipdef.h>
#include "CTEP_Trans_Packet_Protocol.h"

#pragma warning(disable:4482)	//warning C4482: 使用了非标准扩展: 限定名中使用了枚举“EmPacketOperationType”

#define READWRITE_DATA	// 描述一个可读写的数据
#define READONLY_DATA	// 描述一个只读数据
#define OPAQUE_DATA		// 描述一个不可读写的不透明数据

#ifndef _VIRT
#	define _VIRT(x)		virtual x
#	define _VIRT_H		virtual HRESULT
#endif
// 
// 关于文件结构,名称,与导出函数的描述
// 
class  CUserData;
class  CAppChannel;
typedef CAppChannel CAppChl;
class  CTransferChannel;
typedef CTransferChannel CTransChl;

struct ReadWritePacket;
typedef ReadWritePacket RWPacket;

interface ICTEPTransferProtocolCallBack;
interface ICTEPTransferProtocolServer;
interface ICTEPTransferProtocolClient;

typedef ICTEPTransferProtocolCallBack *ICTEPTransferProtocolCallBackPtr;
typedef ICTEPTransferProtocolServer *ICTEPTransferProtocolServerPtr;
typedef ICTEPTransferProtocolClient *ICTEPTransferProtocolClientPtr;

interface ICTEPAppProtocolCallBack;
interface ICTEPAppProtocol;
interface ICTEPAppProtocolEx;

interface ICTEPAppProtocolCrossCallBack;

//	以下函数为Ctep服务器端传输层Dll的导出函数,用于与CtepCommServer对接使用,以提供对特定底层协议的连接支持(如RDP/SPICE等).
//Dll的名称必须以名称: "CtepTs" 开头,并且以dll为后缀名
#define FUNC_CTEPGetInterfaceTransServer	"CTEPGetInterfaceTransServer"
typedef ICTEPTransferProtocolServer* WINAPI T_CTEPGetInterfaceTransServer();
typedef T_CTEPGetInterfaceTransServer * Fn_CTEPGetInterfaceTransServer;

//	以下函数为Ctep客户端传输层Dll的导出函数,用于与CtepCommClient对接使用,以提供对特定底层协议的连接支持(如RDP/SPICE等).
//Dll的名称必须以名称: "CtepTc" 开头,并且以dll为后缀名
#define FUNC_CTEPGetInterfaceTransClient	"CTEPGetInterfaceTransClient"
typedef ICTEPTransferProtocolClient* WINAPI T_CTEPGetInterfaceTransClient();
typedef T_CTEPGetInterfaceTransClient *Fn_CTEPGetInterfaceTransClient;

//	以下函数为Ctep服务器端应用层Dll的导出函数,用于与CtepCommServer对接使用,以实现一个基于CTEP的服务器端应用程序(如CTMMR/USBREDIR).
//Dll的名称必须以名称: "CtepAs" 开头,并且以dll为后缀名
#define FUNC_CTEPGetInterfaceAppServer		"CTEPGetInterfaceAppServer"
typedef ICTEPAppProtocol* WINAPI T_CTEPGetInterfaceAppServer();
typedef T_CTEPGetInterfaceAppServer *Fn_CTEPGetInterfaceAppServer;

//	以下函数为Ctep客户端应用层Dll的导出函数,用于与CtepCommClient对接使用,以实现一个基于CTEP的客户端应用程序(如CTMMR/USBREDIR).
//Dll的名称必须以名称: "CtepAc" 开头,并且以dll为后缀名
#define FUNC_CTEPGetInterfaceAppClient		"CTEPGetInterfaceAppClient"
typedef ICTEPAppProtocol* WINAPI T_CTEPGetInterfaceAppClient();
typedef T_CTEPGetInterfaceAppClient *Fn_CTEPGetInterfaceAppClient;

// 
// 一下用于跨进程的服务器端CTEP代理/存根代码通信
// 

//	以下函数为Ctep服务器端应用层DLL的导出函数,用于与CtepAppProxy对接使用,以实现一个基于CTEP的服务器端应用程序(如CTMMR/USBREDIR).
//Dll的名称必须以名称: "CtepAPxs" 开头,并且以dll为后缀名
#define FUNC_CTEPGetInterfaceAppProxyServer		"CTEPGetInterfaceAppProxyServer"
typedef ICTEPAppProtocol* WINAPI T_CTEPGetInterfaceAppProxyServer();
typedef T_CTEPGetInterfaceAppProxyServer *Fn_CTEPGetInterfaceAppProxyServer;


//	以下函数用于导出跨进程使用的Ctep应用层App使用接口.
#define FUNC_CTEPGetInterfaceAppStubCallBack	"CTEPGetInterfaceAppStubCallBack"
typedef ICTEPAppProtocolCrossCallBack* WINAPI T_CTEPGetInterfaceAppCrossCallBack();
typedef T_CTEPGetInterfaceAppCrossCallBack *Fn_CTEPGetInterfaceAppStubCallBack;





// 
// 一些通用数据结构的描述
// 

// 描述一个指定用户的用户特性数据
enum EmUserType
{
	User_Normal = 0,		// 普通用户类型,用户生命周期由底层链路与上层应用链接共同决定
	User_WinSession = 1,	// windows Session用户类型,生命周期与windows会话保持一致
};
enum EmUserStatus
{
	User_Invalid = -1,		// 当前User处于无效状态,或者已经被回收释放
	User_Disconnected = 0,	// 当前User处于离线状态,用户没有与服务器端建立连接
	User_Connected = 1,		// 当前User已经与用户建立了连接
};
class ATL_NO_VTABLE CUserData
{
public:
	CUserData()
	{
		Init();
	}

	void Init()
	{
		wsUserName[0] = NULL;

		UserId		= INVALID_UID;
		dwSessionId	= INVALID_SESSIONID;
		Type		= (User_Normal);
		Status		= User_Invalid;

		guidUser	= GUID_NULL;

		ZeroObject(addrLocal6);
		ZeroObject(addrRemote6);
	}

	USHORT			UserId;					// -1表示无效
	DWORD			dwSessionId;			// -1表示无效
	WCHAR			wsUserName[260];		// \0表示无效

	GUID			guidUser;
	EmUserType		Type;
	EmUserStatus	Status;

	union
	{
		SOCKADDR_IN  addrLocal;			// 连接的本地地址
		SOCKADDR_IN6 addrLocal6;			// 连接的本地地址
	};

	union
	{
		SOCKADDR_IN addrRemote;			// 连接的远程地址
		SOCKADDR_IN6 addrRemote6;			// 连接的远程地址
	};
};

// 描述一个包操作类型
enum EmPacketOperationType
{
	OP_Empty = 0,
	OP_Listen,			// 监听包,用于监听新的用户连接的包
	OP_IocpSend,		// 完成端口上的发送数据包
	OP_IocpRecv,		// 完成端口上的接收数据包
};

// 内存模组,描述了一块内存地址以及内部的有效数据数量(单位:bytes)
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

// 一个独立的操作包
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
	OPAQUE_DATA CHAR* volatile	pointer;		// 当前处理指针

	union
	{
		READWRITE_DATA ICTEPTransferProtocolServer*	piTrans;
		READWRITE_DATA ICTEPTransferProtocolClient*	piTransClt;
	};
	
	OPAQUE_DATA CTransferChannel * pTransChn;

	OPAQUE_DATA ReadWritePacket  * pNext;

	OPAQUE_DATA CHAR	 cBuf[CTEP_DEFAULT_BUFFER_SIZE];
};



// 
// 一些消息型回调函数的函数格式定义

//默认函数形式,占位使用
typedef void (WINAPI *Call_BaseFunction)(void* pParam);

//windows用户会话消息回调函数格式定义,当有会话消息时,CTEP Comm模块通知其他模组.
//wParam value is:
//	WTS_CONSOLE_CONNECT			0x1 	A session was connected to the console terminal.
//	WTS_CONSOLE_DISCONNECT		0x2		A session was disconnected from the console terminal.
//	WTS_REMOTE_CONNECT			0x3		A session was connected to the remote terminal.
//	WTS_REMOTE_DISCONNECT		0x4		A session was disconnected from the remote terminal.
//	WTS_SESSION_LOGON			0x5		A user has logged on to the session.
//	WTS_SESSION_LOGOFF			0x6		A user has logged off the session.
//	WTS_SESSION_LOCK			0x7		A session has been locked.
//	WTS_SESSION_UNLOCK			0x8		A session has been unlocked.
//	WTS_SESSION_REMOTE_CONTROL	0x9		A session has changed its remote controlled status. To determine the status, call GetSystemMetrics and check the SM_REMOTECONTROL metric.
//lParam vallue is: SessionId.
typedef void (WINAPI *Call_SessionEvent)(void *pParam, WPARAM wParam, LPARAM lParam);


struct StCallEvent
{
	void* pParam;
	union
	{
		Call_BaseFunction  fnBase;
		Call_SessionEvent  fnSessionEvent;
	};

	enum EmEventType
	{
		none = 0,
		SessionEvent,
	}type;
};







