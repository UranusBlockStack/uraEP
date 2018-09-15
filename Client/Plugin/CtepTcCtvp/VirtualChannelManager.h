#pragma once
#include <winsock2.h>
#include "channel-ctvp.h"

#include "Interface\CTEP_Communicate_TransferLayer_Interface.h"
#include "../../CtepCommClient/CtepCommClientExport.h"

#include <ws2tcpip.h>
#include <iphlpapi.h>
#include "psapi.h"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib,"psapi.lib")
class CCTVPVirtualChannelManager : public ICTEPTransferProtocolClient
{
	struct VIRTUAL_CHANNEL_ITEM
	{
		char   m_szChannelName[7];
		LPHANDLE m_phChannel;
		DWORD m_dwChannelNum;
		BOOL m_bChannelOpened;
	};

public:
	CCTVPVirtualChannelManager(PCTVPCHANNEL_ENTRY_POINTS pEntryPoints);
	~CCTVPVirtualChannelManager();

	HANDLE InitChannel(const char* pChannelName = "CTEP");
	//同步发送数据
	BOOL WriteDataToChannel(ReadWritePacket* pPacket);
	void OpenChannelByInitHandle(LPVOID pInitHandle);
	void ChannelWriteComplete(ReadWritePacket* pPacket)
	{
		ASSERT(m_piCallBack);
		if (m_piCallBack == NULL)
			return;

		m_piCallBack->FreePacket(pPacket);
	}
	void CloseChannelByInitHandle(LPVOID pInitHandle);
	//int GetTCPLocalProcessInfo(in_addr *IpLocalAddr,in_addr *IpRemoteAddr,DWORD *dwCurPid);//获取当前进程TCP连接IP地址
	VIRTUAL_CHANNEL_ITEM* GetChannel();

private:
	VIRTUAL_CHANNEL_ITEM* m_Channel;
	PCTVPCHANNEL_ENTRY_POINTS m_pEntryPoints;
	CMyCriticalSection	  m_lckSend;

protected:
	ICTEPTransferProtocolClientCallBack*	m_piCallBack;
	Log4CppLib								m_log;
};
