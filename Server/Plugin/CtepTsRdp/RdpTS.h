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
	_VIRT(LPCSTR) GetName() override {return "RDP";};	// 返回传输协议名称;
	_VIRT_D SupportIOCP() override {return CTEP_TS_SUPPORT_SYNC;};	// 是否支持完成端口模型
	_VIRT_L GetDuration(ReadWritePacket* pPacket) override;		// 判断一个Socket存在的时间,只有TCP实现,其他返回-1;
	_VIRT(SOCKET) GetListenSocket() override; //Only TCP/UDP, 其他返回INVALID_SOCKET(-1)
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
	Log4CppLib m_log;
	CMyCriticalSection csSvr;
};

ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServer();

