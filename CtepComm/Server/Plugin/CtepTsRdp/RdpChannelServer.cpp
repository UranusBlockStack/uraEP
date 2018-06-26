#include "StdAfx.h"
#include "RdpChannelServer.h"
#pragma comment(lib,"Wtsapi32.lib")

HANDLE CRdpChannelServer::RdpChannelHandleToFileHandle(HANDLE m_hChannel)
{
	DWORD           errCode;
	LPVOID          vcFileHandlePtr;
	DWORD           length = 0;
	HANDLE          hFile;

	if( WTSVirtualChannelQuery(m_hChannel, WTSVirtualFileHandle, &vcFileHandlePtr, &length) == FALSE)
	{
		WTSVirtualChannelClose(m_hChannel);
		errCode = GetLastError();
		SetLastError(errCode);
		return INVALID_HANDLE_VALUE;
	}
	hFile = (HANDLE)vcFileHandlePtr;

	WTSFreeMemory(vcFileHandlePtr);
	return hFile;
}

BOOL CRdpChannelServer::RdpChannelCheckProtocol(DWORD m_dwSessionID)
{
	USHORT* pusType;
	DWORD dwRet;
	if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE
		, m_dwSessionID, WTSClientProtocolType, (LPTSTR*)&pusType, &dwRet))
	{
		USHORT usType = *pusType;
		WTSFreeMemory(pusType);

		if (usType == WTS_PROTOCOL_TYPE_RDP)
			return TRUE;
	}

	return FALSE;
}

HANDLE CRdpChannelServer::RdpChannelOpen(DWORD dwSessionID,const char* pChannelName)
{
	OutputDebugString(_T("rdp open channel.."));
	return WTSVirtualChannelOpen(WTS_CURRENT_SERVER_HANDLE, dwSessionID,(char *)pChannelName);
}

BOOL CRdpChannelServer::RdpChannelClose(HANDLE m_hChannel)
{
	return WTSVirtualChannelClose(m_hChannel);
}


BOOL CRdpChannelServer::ChannelAsyncRead(ReadWritePacket *pPacket)
{
	BOOL         bReadSucess;

	bReadSucess = ReadFile(pPacket->hFile, pPacket->buff.buff, pPacket->buff.maxlength, 0, &pPacket->ol);
	if ( bReadSucess || GetLastError() == ERROR_IO_PENDING)
		return TRUE;

	return FALSE;

}
BOOL CRdpChannelServer::ChannelAsyncWrite(ReadWritePacket *pPacket)
{
	BOOL bWriteSucess = WriteFile(pPacket->hFile,pPacket->buff.buff, pPacket->buff.size, 0, &pPacket->ol);
	if ( bWriteSucess || GetLastError() == ERROR_IO_PENDING)
		return TRUE;

	return FALSE;
}

BOOL CRdpChannelServer::RdpChannelCanceIO(HANDLE m_hFile)
{
	return CancelIo(m_hFile);
}
