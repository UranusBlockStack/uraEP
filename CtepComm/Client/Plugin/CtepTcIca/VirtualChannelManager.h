#pragma once

#include "CommonInclude/Tools/LockedContainer.h"
#include "Interface\CTEP_Communicate_TransferLayer_Interface.h"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib,"psapi.lib")

class VIRTUAL_CHANNEL_ITEM
{
public:
	VIRTUAL_CHANNEL_ITEM()
	{
		Clear();
	}
	void Clear()
	{
		ASSERT(listPacketsSent.IsEmpty());

		ZeroArray(szChannelName);
		usVCNum = 0;
		bChannelOpened = FALSE;
		pWd = nullptr;

		pQueueVirtualWrite = nullptr;
		pOutBufReserveProc = nullptr;
		pOutBufAppendProc = nullptr;
		pOutBufWriteProc = nullptr;
		pAppendVdHeaderProc = nullptr;
	}

	char						szChannelName[20];
	USHORT						usVCNum;
	BOOL						bChannelOpened;
	PVOID						pWd;
	CLockList<ReadWritePacket>	listPacketsSent;

	PQUEUEVIRTUALWRITEPROC	pQueueVirtualWrite;

	POUTBUFRESERVEPROC		pOutBufReserveProc;
	POUTBUFAPPENDPROC		pOutBufAppendProc;
	POUTBUFWRITEPROC		pOutBufWriteProc;
	PAPPENDVDHEADERPROC		pAppendVdHeaderProc;
};

class CICAVirtualChannelManager : public VIRTUAL_CHANNEL_ITEM
{
public:
	CICAVirtualChannelManager(PVD pVd);

	HANDLE InitChannel(const char* pChannelName = "CTEP");
	//同步发送数据
	BOOL WriteDataToChannel(ReadWritePacket* pPacket);
	void DriveClose();

	int IcaPollData(PVD pVd, PDLLPOLL pVdPoll, PUINT16 puiSize);
	int SendDataDirect(ReadWritePacket* pPacket);

protected:
	Log4CppLib			  m_log;

public:
	ULONG                 m_ChannelMask;
	PVD                   m_pVd;
	USHORT                m_maxDataSize;

	//VIRTUAL_CHANNEL_ITEM  m_IcaVChnl;
	CMyCriticalSection	  m_lckSend;
};


class CIcaTransferClient : public ICTEPTransferProtocolClient
	, public CICAVirtualChannelManager
{
public:
	CIcaTransferClient(PVD pEntry) : CICAVirtualChannelManager(pEntry), m_piCallBack(nullptr)
		, m_pTransChnn(nullptr){}
		

public:// interface:ICTEPTransferProtocolClient
	virtual LPCSTR GetName() override{ return "ICA";}	// 返回传输协议名称
	virtual HRESULT Initialize(ICTEPTransferProtocolClientCallBack* pI) override;
	virtual void Final() override{;}

	virtual HRESULT Connect(CTransferChannel* pTransChn, ReadWritePacket* pPacket = 0) override;
	virtual HRESULT Disconnect(CTransferChannel* pTransChn) override;

	virtual HRESULT Send(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	virtual HRESULT Recv(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override {return E_NOTIMPL;}

public:
	void RecvData(char* pbuff,DWORD size);

private:
	CMyCriticalSection						lck;
	CTransferChannel*						m_pTransChnn;

	ICTEPTransferProtocolClientCallBack*	m_piCallBack;
};







