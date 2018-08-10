#pragma once
#ifndef __CTEPTCCTVP_H__
#define __CTEPTCCTVP_H__

#include "../../CtepCommClient/CtepCommClientExport.h"

#include "CommonInclude/CommonImpl.h"
#include "Interface\CTEP_Communicate_TransferLayer_Interface.h"
#include "VirtualChannelManager.h"

class CTVPTransferClient : public CCTVPVirtualChannelManager
{
public:
	//公共的
	CTVPTransferClient(PCTVPCHANNEL_ENTRY_POINTS pEntryPoints);
	~CTVPTransferClient();
	virtual LPCSTR GetName() override;	// 返回传输协议名称
	//RDP 初始化
	virtual HRESULT Initialize(ICTEPTransferProtocolClientCallBack* pI) override;
	virtual void Final() override;

	virtual HRESULT Connect(CTransferChannel* pTransChn, ReadWritePacket* pPacket = 0) override;
	virtual HRESULT Disconnect(CTransferChannel* pTransChn) override;

	virtual HRESULT Send(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	virtual HRESULT Recv(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;

public:
	void RecvData(char *pbuff, DWORD size);
private:
	Log4CppLib								m_log;

	CRITICAL_SECTION						cs;
	CTransferChannel*						m_pTransChn;
};






#endif

