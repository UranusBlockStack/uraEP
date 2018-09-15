#pragma once

#include <Wtsapi32.h>

#include "CommonInclude/Tools/LockedContainer.h"

#include "interface\CTEP_Common_struct.h"

class CIcaChannelServer
{
public:
	static HANDLE     IcaChannelHandleToFileHandle(HANDLE m_hChannel);
	static HANDLE     IcaChannelOpen(DWORD m_dwSessionID,const char* pChannelName = "CTEP");
	static BOOL       IcaChannelClose(HANDLE m_hChannel);
	static BOOL       IcaChannelCheckProtocol(DWORD m_dwSessionID);

	//异步读取通道
	static BOOL       ChannelAsyncRead(ReadWritePacket *pPacket);
	static BOOL       ChannelAsyncWrite(ReadWritePacket *pPacket);

	static BOOL       IcaChannelCanceIO(HANDLE m_hChannel);

	static Log4CppLib m_log;
};

_SELECTANY Log4CppLib CIcaChannelServer::m_log("TsIca");

ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServer();
BOOL CtepTsIcaMainThreadInit();
void CtepTsIcaMainThreadClose();

struct StListenData
{
	DWORD  dwMagic;	//0x12344321
	DWORD  dwSessionId;
	HANDLE hRdpChannel;
};


extern ICTEPTransferProtocolCallBack* m_piCallBack;
extern CLockMap<DWORD, CTransferChannel> g_mapSessionData;
extern CMyCriticalSection lckMap;

