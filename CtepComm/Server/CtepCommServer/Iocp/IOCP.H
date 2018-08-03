
#pragma once
#include "CommonInclude/CommonImpl.h"
#include "CommonInclude/Tools/FastQueue.h"

#include "Interface/CTEP_Communicate_App_Interface.h"
#include "Interface/CTEP_Communicate_TransferLayer_Interface.h"
#include "Interface/CTEP_Trans_Packet_Protocol.h"

#include "Interface/CTEP_Common_Struct_Ex.h"

#include "../LoadModules.h"

#include <winsock2.h>
#include <windows.h>
#include <Mswsock.h>

#include <atlcoll.h>
using namespace ATL;

#define MAX_THREAD	32			// I/O服务线程的数量


class CIOCPServer : public ICTEPTransferProtocolCallBack
	, public CLoadModules
{
protected:
	static Log4CppLib m_log;

public:
//interface ICTEPTransferProtocolCallBack
	virtual HANDLE GetCompletePort() override {return m_hCompletion;}	// 返回完成端口句柄
	//virtual HANDLE GetListenEvent() override {return ;}
	virtual ReadWritePacket* AllocatePacket(ICTEPTransferProtocolServer* pI) override 
	{
		ReadWritePacket* pPacket = AllocateBuffer(0, OP_Empty, 0);
		if ( pPacket)
			pPacket->piTrans = pI;
		return pPacket;
	}
	virtual void FreePacket(ReadWritePacket* p) override 
	{
		ReleaseBuffer(p);
	}

public:
	CIOCPServer();
	~CIOCPServer();

	// 开始服务
	BOOL Start(int nMaxConnections = 5000, int nMaxFreeBuffers = 2000);
	// 停止服务
	void Shutdown();

	// 关闭一个连接和关闭所有连接
	void CloseAConnection(CTransferChannelEx *pContext);
	void CloseAllConnections();

	// 取得当前的连接数量
	ULONG GetCurrentConnection()
	{
		//LOCK(&m_lcklstConnection);
		return (ULONG)m_lstConnection.GetCount();
	}

	// 向指定客户发送文本
	BOOL SendText(CTransferChannelEx *pContext, char *pszText, int nLen, EmPacketLevel level = Middle);
	BOOL SendPacket(CTransferChannelEx *pContext, ReadWritePacket* pBuffer, EmPacketLevel level/* = Middle*/);

protected:
	// 申请和释放缓冲区对象
	ReadWritePacket *AllocateBuffer(CTransferChannel* pContext, EmPacketOperationType type, int nLen = 0);
	void ReleaseBuffer(ReadWritePacket *pBuffer);

	// 申请和释放套节字上下文
	CTransferChannelEx *AllocateContext(HANDLE s, ICTEPTransferProtocolServer* piTrans);
	void ReleaseContext(CTransferChannelEx *pContext);

	// 释放空闲缓冲区对象列表和空闲上下文对象列表
	void FreeBuffers();
	void FreeContexts();

	// 向连接列表中添加一个连接
	BOOL AddAConnection(CTransferChannelEx *pContext);

	// 插入和移除未决的接受请求
	BOOL InsertPendingAccept(ReadWritePacket *pBuffer);
	BOOL RemovePendingAccept(ReadWritePacket *pBuffer);

	void HandleIO(CTransferChannelEx *pContext, ReadWritePacket *pBuffer, DWORD dwTrans, int nError);
	void OnWriteCompleted(CTransferChannelEx *pContext, ReadWritePacket *pBuffer);// 一个连接上的写操作完成
	BOOL sendMQUEPacket(ReadWritePacket* pSend);


// 事件通知函数
	// 建立了一个新的连接
	virtual void OnConnectionEstablished(CTransferChannelEx *pContext, ReadWritePacket *pBuffer) = 0;
	// 一个连接关闭
	virtual void OnConnectionClosing(CTransferChannelEx *pContext, ReadWritePacket *pBuffer) = 0;
	// 在一个连接上发生了错误
	virtual BOOL OnConnectionError(CTransferChannelEx * /*pContext*/, ReadWritePacket * /*pBuffer*/, int /*nError*/){return FALSE;}
	// 一个连接上的读操作完成
	virtual void OnReadCompleted(CTransferChannelEx *pContext, ReadWritePacket *pBuffer) = 0;
	// 服务器启动
	virtual BOOL OnStart() = 0;
	// 服务器关闭
	virtual void OnShutdown() = 0;

protected:
	// 通道发送分级结构信息
#define MQUE_MAX_SEND	500
#define MQUE_MIN_SEND	100
	MutliPriorityQueue<ReadWritePacket> m_mqueSendPriority;
	ThreadLock							m_mqueSPLock;


	// 记录空闲结构信息
	FastQueue<ReadWritePacket> m_queFreePacket;
	FastQueue<CTransferChannelEx> m_queFreeContext;

	// 记录抛出的Accept请求
	FastQueue<ReadWritePacket> m_quePendingAccepts;   // 抛出请求列表。

	// 记录已经建立的连接列表
	CAtlList<CTransferChannelEx*> m_lstConnection;
	CMyCriticalSection           m_lcklstConnection;

	// 记录等待执行同步读取操作的列表
	MISOQueue<CTransferChannelEx> m_queSyncRead;
	static DWORD WINAPI _SyncWorkerThreadProc(LPVOID lpParam){return ((CIOCPServer*)lpParam)->_threadSyncWorker();}
	DWORD _threadSyncWorker();
	HANDLE m_hSyncWorkThread;


	// 用于投递Accept请求
	HANDLE m_hAcceptEvent;
	HANDLE m_hRepostEvent;
	LONG   m_nRepostCount;

	DWORD  m_dwWorkThreadCount;	// 记录工作线程数量

	int m_nMaxAccepts;
	int m_nMaxSends;
	int m_nMaxFreeBuffers;
	int m_nMaxConnections;

	HANDLE m_hListenThread;			// 监听线程
	HANDLE m_hCompletion;			// 完成端口句柄

	volatile long m_nIocpOutStandingSend;
	volatile long m_nIocpOutStandingRecv;
	volatile long m_nIocpOutStandingListen;

	volatile BOOL m_bShutDown;		// 用于通知监听线程退出
	volatile BOOL m_bServerStarted;	// 记录服务是否启动

private:	// 线程函数
	DWORD _threadListen();
	static DWORD WINAPI _WorkerThreadProc(LPVOID lpParam){return ((CIOCPServer*)lpParam)->_threadIocpWorker();}
	DWORD _threadIocpWorker();
};

