#pragma once

#include "Interface/CTEP_Communicate_TransferLayer_Interface.h"
#include "CommonInclude/Tools/ProcessThread.h"

#define CTMMR_PIPE_NAME_TEMPLATE			TEXT("\\\\.\\pipe\\MMrPipeName-%d")
#define CTEPTS_CTVP_PIPE_NAME_TEMPLATE		CTMMR_PIPE_NAME_TEMPLATE

#define SVRNAMEDPIPE_LOW_SECURITY_PIPE		TRUE

class CTransProCtvpSvr : public ICTEPTransferProtocolServer
{
public:
	CTransProCtvpSvr()
		:m_log("TS_CTVP")
		, m_hPipeCurrCtvp(INVALID_HANDLE_VALUE)
		, m_hPipeWaitConnect(INVALID_HANDLE_VALUE)
		, m_piCallBack(NULL)
	{
		m_Worker.Initialize(this, &CTransProCtvpSvr::_MonitorPipeThread);
	}

public:
	_VIRT(LPCSTR) GetName() override { return "CTVP";}
	_VIRT_D SupportIOCP() override { return CTEP_TS_SUPPORT_IOCP; };
	_VIRT(SOCKET) GetListenSocket() override {return INVALID_SOCKET;}; //Only TCP/UDP, 其他返回INVALID_SOCKET(-1)
	_VIRT_L GetDuration(ReadWritePacket* pPacket) override {return -1;};
	_VIRT_H InitializeCompletePort(ICTEPTransferProtocolCallBack* piCallBack) override;
	_VIRT_H PostListen(bool bFirst = false) override;
	_VIRT_H CompleteListen(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	_VIRT_H PostRecv(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	_VIRT_H PostSend(CTransferChannel* pTransChn, ReadWritePacket* pPacket)  override;
	_VIRT_H Disconnect(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	_VIRT_V Final() override;
	// Tcp/Rdp Only
	_VIRT(WORD) GetPort() override {return 0;}	// 返回TCP/RDP监听端口号


private:
	typedef CWorkerThread<CTransProCtvpSvr> MyWorker;
	MyWorker m_Worker;
	DWORD _MonitorPipeThread(LPVOID param, HANDLE hEventMessage, MyWorker* pWrkTrd);

private:
	Log4CppLib m_log;

	CMyCriticalSection	m_lck;
	HANDLE				m_hPipeCurrCtvp;
	HANDLE				m_hPipeWaitConnect;
	ICTEPTransferProtocolCallBack* m_piCallBack;
};

ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServer();

