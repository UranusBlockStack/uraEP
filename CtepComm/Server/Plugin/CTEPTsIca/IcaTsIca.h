#pragma once

#include "Interface/CTEP_Communicate_TransferLayer_Interface.h"
#include "WFAPI/Include/wfapi.h"
#define DEFAULT_PORT 4567

#include <Wtsapi32.h>
#include "IcaChannelServer.h"
#pragma comment (lib,"Wtsapi32.lib")
#pragma comment (lib,"wfapi.lib")
class CTransProIcaSvr: public ICTEPTransferProtocolServer
{
public:
	virtual LPCSTR GetName() override { return "ICA" ;};
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