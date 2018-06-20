#ifndef __CTEPTSTCP_H__
#define __CTEPTSTCP_H__
#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>
#pragma comment(lib,"WS2_32.lib")
#include "Interface/CTEP_Communicate_TransferLayer_Interface.h"

#define DEFAULT_PORT              4567
#define MAX_LISTEN_CONNECTION     200
class CTransProTcpSvr : public ICTEPTransferProtocolServer
{
public:
	CTransProTcpSvr();
	~CTransProTcpSvr();

public:
	virtual bool SupportIOCP() override{return TRUE;}	// 是否支持完成端口模型
	virtual LPCSTR GetName() override {return "TCP";}	// 返回传输协议名称;
	virtual SOCKET GetListenSocket() override;

	// 支持完成端口
	virtual HRESULT InitializeCompletePort(ICTEPTransferProtocolCallBack* piCallBack) override;
	virtual HRESULT PostListen(bool bFirst = false) override;
	
	virtual HRESULT PostRecv(StTransferChannel* pTransChn, bool bFirst = false) override;
	virtual HRESULT PostSend(StTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	virtual HRESULT CompleteListen(StTransferChannel* pTransChn, ReadWritePacket* pPacket) override;

	// 不支持完成端口的
	virtual HRESULT Initialize(ICTEPTransferProtocolCallBack* piCallBack) override;
	virtual HRESULT Listen(StTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	virtual HRESULT Recv(StTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	virtual HRESULT Send(StTransferChannel* pTransChn, ReadWritePacket* pPacket) override;

	//公共的
	virtual long CTransProTcpSvr::GetDuration(ReadWritePacket* pPacket) override;
	virtual HRESULT Disconnect(StTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	virtual void Final() override;

	// Tcp/Rdp Only
	virtual USHORT GetPort() override {return m_uPort;}	// 返回TCP/RDP监听端口号


private:
	Log4CppLib m_log;

	USHORT m_uPort;
	ICTEPTransferProtocolCallBack* m_piCallBack;

	SOCKET m_sListen;
	CRITICAL_SECTION cs;
	LPFN_ACCEPTEX m_lpfnAcceptEx;	// AcceptEx函数地址
	LPFN_GETACCEPTEXSOCKADDRS m_lpfnGetAcceptExSockaddrs; // GetAcceptExSockaddrs函数地址
};

ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServer();

#endif