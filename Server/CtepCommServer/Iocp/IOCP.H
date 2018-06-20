
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
	virtual HANDLE GetListenEvent() override {return m_hListenThread;}
	//virtual BOOL InsertPendingAccept(ReadWritePacket *pBuffer) = 0;
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
	void CloseAConnection(StTransferChannelEx *pContext);
	void CloseAllConnections();

	// 取得当前的连接数量
	ULONG GetCurrentConnection() { return m_queConnectionList.GetCount(); }

	// 向指定客户发送文本
	BOOL SendText(StTransferChannelEx *pContext, char *pszText, int nLen, EmPacketLevel level = Middle);
	BOOL SendPacket(StTransferChannelEx *pContext, ReadWritePacket* pBuffer, EmPacketLevel level = Middle);

protected:
	// 申请和释放缓冲区对象
	ReadWritePacket *AllocateBuffer(StTransferChannel* pContext, EmPacketOperationType type, int nLen = 0);
	void ReleaseBuffer(ReadWritePacket *pBuffer);

	// 申请和释放套节字上下文
	StTransferChannelEx *AllocateContext(HANDLE s, ICTEPTransferProtocolServer* piTrans);
	//void ReleaseContext(StTransferChannelEx *pContext);// 功能被整合进CloseAConnection

	// 释放空闲缓冲区对象列表和空闲上下文对象列表
	void FreeBuffers();
	void FreeContexts();

	// 向连接列表中添加一个连接
	BOOL AddAConnection(StTransferChannelEx *pContext);

	// 插入和移除未决的接受请求
	BOOL InsertPendingAccept(ReadWritePacket *pBuffer);
	BOOL RemovePendingAccept(ReadWritePacket *pBuffer);

	void HandleIO(StTransferChannelEx *pContext, ReadWritePacket *pBuffer, DWORD dwTrans, int nError);
	void OnWriteCompleted(StTransferChannelEx *pContext, ReadWritePacket *pBuffer);// 一个连接上的写操作完成
	BOOL sendMQUEPacket(ReadWritePacket* pSend);


// 事件通知函数
	// 建立了一个新的连接
	virtual void OnConnectionEstablished(StTransferChannelEx *pContext, ReadWritePacket *pBuffer) = 0;
	// 一个连接关闭
	virtual void OnConnectionClosing(StTransferChannelEx *pContext, ReadWritePacket *pBuffer) = 0;
	// 在一个连接上发生了错误
	virtual BOOL OnConnectionError(StTransferChannelEx * /*pContext*/, ReadWritePacket * /*pBuffer*/, int /*nError*/){return FALSE;}
	// 一个连接上的读操作完成
	virtual void OnReadCompleted(StTransferChannelEx *pContext, ReadWritePacket *pBuffer) = 0;
	// 服务器启动
	virtual BOOL OnStart() = 0;
	// 服务器关闭
	virtual void OnShutdown() = 0;

protected:
	// 通道发送分级结构信息
#define MQUE_MAX_SEND	500
#define MQUE_MIN_SEND	200
	MutliPriorityQueue<ReadWritePacket> m_mqueSendPriority;
	ThreadLock							m_mqueSPLock;


	// 记录空闲结构信息
	FastQueue<ReadWritePacket> m_queFreePacket;
	FastQueue<StTransferChannelEx> m_queFreeContext;

	// 记录抛出的Accept请求
	FastQueue<ReadWritePacket> m_quePendingAccepts;   // 抛出请求列表。

	// 记录连接列表
	FastQueue<StTransferChannelEx> m_queConnectionList;

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

	long volatile m_nIocpOutStandingSend;
	long volatile m_nIocpOutStandingRecv;
	long volatile m_nIocpOutStandingListen;

	BOOL m_bShutDown;		// 用于通知监听线程退出
	BOOL m_bServerStarted;	// 记录服务是否启动

private:	// 线程函数
	static DWORD WINAPI _ListenThreadProc(LPVOID lpParam){return ((CIOCPServer*)lpParam)->_threadListen();}
	DWORD _threadListen();
	static DWORD WINAPI _WorkerThreadProc(LPVOID lpParam){return ((CIOCPServer*)lpParam)->_threadWorker();}
	DWORD _threadWorker();
};

