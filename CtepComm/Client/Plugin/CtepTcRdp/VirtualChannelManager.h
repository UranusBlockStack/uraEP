#pragma once

#include "cchannel.h"
#include "pchannel.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include "windows.h"
#include "tchar.h"
#include "psapi.h"


#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib,"psapi.lib")

#include "Interface\CTEP_Communicate_TransferLayer_Interface.h"
#include "../../CtepCommClient/CtepCommClientExport.h"

class CVirtualChannelManager : public ICTEPTransferProtocolClient
{
	struct VIRTUAL_CHANNEL_ITEM
	{
		char   m_szChannelName[7];
		LPHANDLE m_phChannel;
		DWORD m_dwChannelNum;
		BOOL m_bChannelOpened;
	};

public:
	CVirtualChannelManager(PCHANNEL_ENTRY_POINTS pEntryPoints);
	~CVirtualChannelManager();

	HANDLE InitChannel(const char* pChannelName = "CTEP");
	//同步发送数据
	BOOL WriteDataToChannel(ReadWritePacket* pPacket);
	//void HandleChannelData(DWORD dwChannel,char* pData,DWORD dwSize);
	void OpenChannelByInitHandle(LPVOID pInitHandle);
	void ChannelWriteComplete(ReadWritePacket* pPacket)
	{
		m_piCallBack->FreePacket(pPacket);
	}
	void CloseChannelByInitHandle(LPVOID pInitHandle);
	int GetTCPLocalProcessInfo(in_addr *IpLocalAddr,in_addr *IpRemoteAddr,DWORD *dwCurPid);
	VIRTUAL_CHANNEL_ITEM* GetChannel();

private:
	VIRTUAL_CHANNEL_ITEM* m_Channel;
	PCHANNEL_ENTRY_POINTS m_pEntryPoints;
	CMyCriticalSection	  m_lckSend;
protected:
	ICTEPTransferProtocolClientCallBack*	m_piCallBack;
};

