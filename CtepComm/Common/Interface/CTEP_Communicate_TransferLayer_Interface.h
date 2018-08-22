  #pragma once

#include <Ws2tcpip.h>
#include "CTEP_Common_struct.h"

enum EnTransferChannelType
{
	TransType_Empty = 0,

	TransType_TCP,			//TCP socket连接,可作为主通道和辅助通道;
	TransType_UDP,			//UDP socket连接,只能作为辅助通道,目前未实现

	TransType_IocpMain,		//支持完成端口类型的主通道,如:CTVP,管道等
	TransType_SyncMain,		//不支持完成端口的主通道,使用同步传输, 如:ICA,RDP等
	TransType_AsyncMain,	//不支持完成端口的主通道,但支持异步传输

	TransType_CrossApp,		//服务器端跨进程动态应用通道专用底层通道,用支持IOCP的命名管道实现
};

class __declspec(novtable) CTransferChannel	// 一条底层传输通道, 表示与一个用户会话的连接
{
public:
	CTransferChannel()
	{
		::InitializeCriticalSection(&lckSend);
	}
	~CTransferChannel()
	{
		::DeleteCriticalSection(&lckSend);
	}
	void Init()
	{
		bClosing = TRUE;

		pUser = nullptr;
		piTrans = nullptr;

		dwSessionId = (DWORD)-1; pParam = 0;
		hFile = INVALID_HANDLE_VALUE;
		ZeroObject(addrLocal6);
		ZeroObject(addrRemote6);
		type = EnTransferChannelType::TransType_Empty;
	}
	READWRITE_DATA DWORD dwSessionId;
	READWRITE_DATA void *pParam;
	union
	{
		READWRITE_DATA HANDLE hFile;
		READWRITE_DATA SOCKET s;
	};

	union
	{
		READWRITE_DATA SOCKADDR_IN  addrLocal;			// 连接的本地地址
		READWRITE_DATA SOCKADDR_IN6 addrLocal6;			// 连接的本地地址
	};
	
	union
	{
		READWRITE_DATA SOCKADDR_IN addrRemote;			// 连接的远程地址
		READWRITE_DATA SOCKADDR_IN6 addrRemote6;			// 连接的远程地址
	};
	
	volatile BOOL bClosing;					// 套节字是否关闭
	EnTransferChannelType type;
	CRITICAL_SECTION lckSend;			// 发送锁

	volatile CUserData			*pUser;
	union
	{
		ICTEPTransferProtocolServer *piTrans;
		ICTEPTransferProtocolClient *piTransClt;
	};

public:
	inline LPWSTR debugEnTransfaerChannelType()
	{
		switch(type)
		{
		case TransType_Empty:
			return L"TransType_Empty";
		case TransType_TCP:
			return L"TransType_TCP";
		case TransType_UDP:
			return L"TransType_UDP";
		case TransType_IocpMain:
			return L"TransType_IocpMain";
		case TransType_SyncMain:
			return L"TransType_SyncMain";
		case TransType_CrossApp:
			return L"TransType_CrossApp";
		}
		return L"EnTransferChannelType??????";
	}
};

// CTEP底层传输层 Server端回调接口
interface ICTEPTransferProtocolCallBack
{
	//支持完成端口
	virtual HANDLE GetCompletePort() = 0;	// 返回完成端口句柄
	virtual BOOL InsertPendingAccept(ReadWritePacket *pBuffer) = 0;	//插入等待用户数据链表,避免出现空数据连接连入
																	//,使用这个方法时必须实现ICTEPTransferProtocolServer接口的GetDuration方法

	virtual ReadWritePacket* AllocatePacket(ICTEPTransferProtocolServer* pI) = 0;
	virtual void FreePacket(ReadWritePacket* ) = 0;

	// CTEP 2.0
	virtual HRESULT RegisterCallBackEvent(StCallEvent Calls[], DWORD dwCallCount) = 0;
	virtual HRESULT UnregisterCallBackEvent(StCallEvent Calls[], DWORD dwCallCount) = 0;
};

interface ICTEPTransferProtocolServer
{
	virtual LPCSTR GetName() = 0;	// 返回传输协议名称

#define CTEP_TS_SUPPORT_IOCP		1
#define CTEP_TS_SUPPORT_ASYNC		2
#define CTEP_TS_SUPPORT_SYNC		0
	virtual DWORD SupportIOCP() = 0;	// 是否支持完成端口模型
	virtual long GetDuration(ReadWritePacket* pPacket) = 0;		// 判断一个Socket存在的时间,只有TCP实现,其他返回-1;
	virtual SOCKET GetListenSocket() = 0; //Only TCP/UDP, 其他返回INVALID_SOCKET(-1)

	virtual HRESULT InitializeCompletePort(ICTEPTransferProtocolCallBack* piCallBack) = 0;
	virtual HRESULT PostListen(bool bFirst = false) = 0;
	virtual HRESULT CompleteListen(CTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;
	virtual HRESULT PostRecv(CTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;
	virtual HRESULT PostSend(CTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;

	virtual HRESULT Disconnect(CTransferChannel* pTransChn, ReadWritePacket* pPacket = 0) = 0;
	virtual void Final() = 0;

	// Tcp/Rdp Only
	virtual USHORT GetPort() = 0;	// 返回TCP/RDP监听端口号
};



interface ICTEPTransferProtocolClientCallBack
{
	virtual HRESULT Connected(CTransferChannel* pTransChn) = 0;
	virtual HRESULT Disconnected(CTransferChannel* pTransChn, DWORD dwErr) = 0;
	virtual HRESULT Recv(CTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;

	virtual ReadWritePacket* AllocatePacket(ICTEPTransferProtocolClient* pI) = 0;
	virtual void FreePacket(ReadWritePacket* ) = 0;
};

interface ICTEPTransferProtocolClient	// 待补充...
{
	//公共的
	virtual LPCSTR GetName() = 0;	// 返回传输协议名称

	virtual HRESULT Initialize(ICTEPTransferProtocolClientCallBack* pI) = 0;
	virtual void Final() = 0;

	virtual HRESULT Connect(CTransferChannel* pTransChn, ReadWritePacket* pPacket = 0) = 0;// RDP返回: E_NOTIMPL
	virtual HRESULT Disconnect(CTransferChannel* pTransChn) = 0;// RDP返回: E_NOTIMPL

	virtual HRESULT Send(CTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;
	virtual HRESULT Recv(CTransferChannel* pTransChn, ReadWritePacket* pPacket) = 0;	// TCP/UDP, RDP不支持,返回E_NOIMPL
};



