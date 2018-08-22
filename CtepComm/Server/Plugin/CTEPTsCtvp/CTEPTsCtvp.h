#pragma once

#include "Interface/CTEP_Communicate_TransferLayer_Interface.h"

#define CTMMR_PIPE_NAME_TEMPLATE			TEXT("\\\\.\\pipe\\CloudtimesMMrPipeName-%d")
#define CTEPTS_CTVP_PIPE_NAME_TEMPLATE		CTMMR_PIPE_NAME_TEMPLATE

#define SVRNAMEDPIPE_LOW_SECURITY_PIPE		TRUE

class CTransProCtvpSvr : public ICTEPTransferProtocolServer
{
public:
	CTransProCtvpSvr()
		:m_log("TS_CTVP")
		, h_MonitorThread(NULL)
		, m_hMmrPipe(INVALID_HANDLE_VALUE)
		, m_piCallBack(NULL)
	{
	}

public:
	virtual LPCSTR GetName() override { return "CTVP";}
	virtual DWORD SupportIOCP() override { return CTEP_TS_SUPPORT_IOCP; };
	virtual SOCKET GetListenSocket() override {return INVALID_SOCKET;}; //Only TCP/UDP, 其他返回INVALID_SOCKET(-1)
	virtual long GetDuration(ReadWritePacket* pPacket) override {return -1;};
	virtual HRESULT InitializeCompletePort(ICTEPTransferProtocolCallBack* piCallBack) override;
	virtual HRESULT PostListen(bool bFirst = false) override;
	virtual HRESULT CompleteListen(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	virtual HRESULT PostRecv(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	virtual HRESULT PostSend(CTransferChannel* pTransChn, ReadWritePacket* pPacket)  override;
	virtual HRESULT Disconnect(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	virtual void Final() override;
	// Tcp/Rdp Only
	virtual USHORT GetPort() override {return 0;}	// 返回TCP/RDP监听端口号


private:
	BOOL ctepTsCtvpMainThreadInit();
	void ctepTsCtvpMainThreadClose();
	static DWORD WINAPI _trdMonitorPipeThread(LPVOID pThis)
	{
		ASSERT(pThis);
		return ((CTransProCtvpSvr*)pThis)->_MonitorPipeThread();
	}
	DWORD _MonitorPipeThread();

private:
	Log4CppLib m_log;

	CMyCriticalSection	m_lck;
	HANDLE				h_MonitorThread;
	HANDLE				m_hMmrPipe;
	ICTEPTransferProtocolCallBack* m_piCallBack;
};

ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServer();

