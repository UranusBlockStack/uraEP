#pragma once

#include "Interface/CTEP_Communicate_TransferLayer_Interface.h"

#include "CommonInclude/Tools/ProcessThread.h"

#define SVRNAMEDPIPE_MAX_PACKET_SIZE		8*1024

#include "CommonInclude/Tools/UserPrivilege.h"
#define SVRNAMEDPIPE_LOW_SECURITY_PIPE		TRUE

class CTransSvrCrossApp : public ICTEPTransferProtocolServer
{
public:
	CTransSvrCrossApp()
		: m_log("TS_CROSS")
		, m_piCallBack(NULL)
	{
		m_TrdPipeMonitor.Initialize(this, &CTransSvrCrossApp::_MonitorPipeThread);
	}

public:
	_VIRT(LPCSTR)  GetName() override { return "TSCROSS";}
	_VIRT_D        SupportIOCP() override { return CTEP_TS_SUPPORT_IOCP; };
	_VIRT(SOCKET)  GetListenSocket() override {return INVALID_SOCKET;}; //Only TCP/UDP, 其他返回INVALID_SOCKET(-1)
	_VIRT_L GetDuration(ReadWritePacket* ) override {return -1;};
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
	CWorkerThread<CTransSvrCrossApp>	m_TrdPipeMonitor;
	DWORD _MonitorPipeThread(LPVOID param, HANDLE hEventMessage, CWorkerThread<CTransSvrCrossApp>* pWrkTrd);

private:
	Log4CppLib m_log;
	//CMyCriticalSection	m_lck;

	ICTEPTransferProtocolCallBack* m_piCallBack;
};

ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServerCrossApp();

