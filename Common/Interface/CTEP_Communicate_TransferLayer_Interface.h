#pragma once

#include <Ws2tcpip.h>
#include "CTEP_Common_struct.h"

enum EnTransferChannelType
{
	TransTypeEmpty = 0,
	TCP,
	UDP,
	IocpMain,
	SyncMain,
};

inline LPWSTR debugEnTransfaerChannelType(EnTransferChannelType type)
{
	switch(type)
	{
	case TransTypeEmpty:
		return L"TransTypeEmpty";
	case TCP:
		return L"TCP";
	case UDP:
		return L"UDP";
	case IocpMain:
		return L"IocpMain";
	case SyncMain:
		return L"SyncMain";

	}
	return L"EnTransferChannelType??????";
}

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
		type = EnTransferChannelType::TransTypeEmpty;
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
};


interface ICTEPTransferProtocolCallBack
{
	//支持完成端口
	virtual HANDLE GetCompletePort() = 0;	// 返回完成端口句柄
	//virtual HANDLE GetListenEvent() = 0;	// only TCP
	virtual BOOL InsertPendingAccept(ReadWritePacket *pBuffer) = 0;

	virtual ReadWritePacket* AllocatePacket(ICTEPTransferProtocolServer* pI) = 0;
	virtual void FreePacket(ReadWritePacket* ) = 0;
};

interface ICTEPTransferProtocolServer
{
	virtual LPCSTR GetName() = 0;	// 返回传输协议名称

#define CTEP_TS_SUPPORT_IOCP		1
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



