#pragma once

#include <Windows.h>
#include <MSWSock.h>
#include "RdpChannelServer.h"
#include "Interface/CTEP_Communicate_TransferLayer_Interface.h"

#define DEFAULT_PORT              4567
#define MAX_LISTEN_CONNECTION     200

class CTransProRdpSvr : public ICTEPTransferProtocolServer
{
public:
	virtual LPCSTR GetName() override {return "RDP";};	// 返回传输协议名称;
	virtual DWORD SupportIOCP() override {return CTEP_TS_SUPPORT_SYNC;};	// 是否支持完成端口模型
	virtual long GetDuration(ReadWritePacket* pPacket) override;		// 判断一个Socket存在的时间,只有TCP实现,其他返回-1;
	virtual SOCKET GetListenSocket() override; //Only TCP/UDP, 其他返回INVALID_SOCKET(-1)
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
	Log4CppLib m_log;
	CMyCriticalSection csSvr;
};

ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServer();

