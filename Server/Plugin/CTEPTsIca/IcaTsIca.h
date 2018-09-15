#pragma once

#include "Interface/CTEP_Communicate_TransferLayer_Interface.h"

#include "WFAPI/Include/wfapi.h"
typedef decltype(WFQuerySessionInformation)		*PFN_WFQuerySessionInformation		;
typedef decltype(WFVirtualChannelQuery)			*PFN_WFVirtualChannelQuery			;
typedef decltype(WFVirtualChannelClose)			*PFN_WFVirtualChannelClose			;
typedef decltype(WFFreeMemory)					*PFN_WFFreeMemory					;
typedef decltype(WFVirtualChannelOpen)			*PFN_WFVirtualChannelOpen			;
typedef decltype(WFVirtualChannelRead)			*PFN_WFVirtualChannelRead			;
typedef decltype(WFVirtualChannelWrite)			*PFN_WFVirtualChannelWrite			;

struct InterfaceIcaVirtualChannelApi
{
	static PFN_WFQuerySessionInformation		pWFQuerySessionInformation			;
	static PFN_WFVirtualChannelQuery			pWFVirtualChannelQuery				;
	static PFN_WFVirtualChannelClose			pWFVirtualChannelClose				;
	static PFN_WFFreeMemory						pWFFreeMemory						;
	static PFN_WFVirtualChannelOpen				pWFVirtualChannelOpen				;
	static PFN_WFVirtualChannelRead				pWFVirtualChannelRead				;
	static PFN_WFVirtualChannelWrite			pWFVirtualChannelWrite				;

	static BOOL LoadFunction()
	{
		if ( hMod)
			return TRUE;

		hMod = LoadLibrary(L"wfapi.dll");
		if ( !hMod)
			return FALSE;

		pWFQuerySessionInformation	= (PFN_WFQuerySessionInformation)GetProcAddress(hMod, "WFQuerySessionInformation");
		pWFVirtualChannelQuery		= (PFN_WFVirtualChannelQuery	)GetProcAddress(hMod, "WFVirtualChannelQuery");
		pWFVirtualChannelClose		= (PFN_WFVirtualChannelClose	)GetProcAddress(hMod, "WFVirtualChannelClose");
		pWFFreeMemory				= (PFN_WFFreeMemory				)GetProcAddress(hMod, "WFFreeMemory");
		pWFVirtualChannelOpen		= (PFN_WFVirtualChannelOpen		)GetProcAddress(hMod, "WFVirtualChannelOpen");
		pWFVirtualChannelRead		= (PFN_WFVirtualChannelRead		)GetProcAddress(hMod, "WFVirtualChannelRead");
		pWFVirtualChannelWrite		= (PFN_WFVirtualChannelWrite	)GetProcAddress(hMod, "WFVirtualChannelWrite");

		if (
			!pWFQuerySessionInformation	  ||
			!pWFVirtualChannelQuery		  ||
			!pWFVirtualChannelClose		  ||
			!pWFFreeMemory				  ||
			!pWFVirtualChannelOpen		  ||
			!pWFVirtualChannelRead		  ||
			!pWFVirtualChannelWrite		
			)
		{
			FreeLibrary(hMod);
			hMod = NULL;
			return FALSE;
		}

		return TRUE;
	}
	static HMODULE hMod;
};
typedef InterfaceIcaVirtualChannelApi WFAPI;

_SELECTANY PFN_WFQuerySessionInformation		WFAPI::pWFQuerySessionInformation;
_SELECTANY PFN_WFVirtualChannelQuery			WFAPI::pWFVirtualChannelQuery;
_SELECTANY PFN_WFVirtualChannelClose			WFAPI::pWFVirtualChannelClose;
_SELECTANY PFN_WFFreeMemory						WFAPI::pWFFreeMemory;
_SELECTANY PFN_WFVirtualChannelOpen				WFAPI::pWFVirtualChannelOpen;
_SELECTANY PFN_WFVirtualChannelRead				WFAPI::pWFVirtualChannelRead;
_SELECTANY PFN_WFVirtualChannelWrite			WFAPI::pWFVirtualChannelWrite;
_SELECTANY HMODULE WFAPI::hMod = NULL;



#define DEFAULT_PORT 4567

#include <Wtsapi32.h>
#include "IcaChannelServer.h"
#pragma comment (lib,"Wtsapi32.lib")

class CTransProIcaSvr: public ICTEPTransferProtocolServer
{
public:
	_VIRT(LPCSTR) GetName() override { return "ICA" ;};
	_VIRT_D SupportIOCP() override {return CTEP_TS_SUPPORT_SYNC;};	// 是否支持完成端口模型
	_VIRT_L  GetDuration(ReadWritePacket* pPacket) override;		// 判断一个Socket存在的时间,只有TCP实现,其他返回-1;
	_VIRT(SOCKET) GetListenSocket() override; //Only TCP/UDP, 其他返回INVALID_SOCKET(-1)
	_VIRT_H InitializeCompletePort(ICTEPTransferProtocolCallBack* piCallBack) override;
	_VIRT_H PostListen(bool bFirst = false) override;
	_VIRT_H CompleteListen(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	_VIRT_H PostRecv(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	_VIRT_H PostSend(CTransferChannel* pTransChn, ReadWritePacket* pPacket)  override;

	_VIRT_H Disconnect(CTransferChannel* pTransChn, ReadWritePacket* pPacket) override;
	_VIRT_V Final() override;

	// Tcp/Rdp Only
	_VIRT(USHORT) GetPort() override {return 0;}	// 返回TCP/RDP监听端口号
private:
	Log4CppLib m_log;
	CMyCriticalSection csSvr;
};


ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServer();