#pragma once

#include <Wtsapi32.h>
#include <atlcoll.h>
#include "interface\CTEP_Common_struct.h"
class CRdpChannelServer
{
public:
	static HANDLE     RdpChannelHandleToFileHandle(HANDLE m_hChannel);
	static HANDLE     RdpChannelOpen(DWORD m_dwSessionID,const char* pChannelName = "CTEP");
	static BOOL       RdpChannelClose(HANDLE m_hChannel);
	static BOOL       RdpChannelCheckProtocol(DWORD m_dwSessionID);

	//异步读取通道
	static BOOL       ChannelAsyncRead(ReadWritePacket *pPacket);
	static BOOL       ChannelAsyncWrite(ReadWritePacket *pPacket);

	static BOOL       RdpChannelCanceIO(HANDLE m_hChannel);
};



ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServer();
BOOL CtepTsRdpMainThreadInit();
void CtepTsRdpMainThreadClose();

struct StListenData
{
	DWORD  dwMagic;	//0x12344321
	DWORD  dwSessionId;
	HANDLE hRdpChannel;
};


extern ICTEPTransferProtocolCallBack* m_piCallBack;
extern CAtlMap<DWORD, CTransferChannel* > g_mapSessionData;
extern CMyCriticalSection lckMap;

