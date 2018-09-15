#include "StdAfx.h"
#include "IcaChannelServer.h"
#include "IcaTsIca.h"


HANDLE CIcaChannelServer::IcaChannelHandleToFileHandle(HANDLE m_hChannel)
{
	DWORD           errCode;
	LPVOID          vcFileHandlePtr;
	DWORD           length = 0;
	HANDLE          hFile;

	if( WFAPI::pWFVirtualChannelQuery(m_hChannel, WFVirtualClientData, &vcFileHandlePtr, &length) == FALSE)
	{
		WFAPI::pWFVirtualChannelClose(m_hChannel);
		errCode = GetLastError();
		SetLastError(errCode);
		return INVALID_HANDLE_VALUE;
	}
	hFile = (HANDLE)vcFileHandlePtr;

	WFAPI::pWFFreeMemory(vcFileHandlePtr);
	return hFile;
}

BOOL CIcaChannelServer::IcaChannelCheckProtocol(DWORD m_dwSessionID)
{
	USHORT* pusType;
	DWORD dwRet;
	if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE
		, m_dwSessionID, WTSClientProtocolType, (LPTSTR*)&pusType, &dwRet))
	{
		USHORT usType = *pusType;
		WTSFreeMemory(pusType);

		if (usType == WTS_PROTOCOL_TYPE_ICA)
			return TRUE;
	}

	return FALSE;
}

HANDLE CIcaChannelServer::IcaChannelOpen(DWORD dwSessionID,const char* pChannelName)
{
	HANDLE hIcaChannel = WFAPI::pWFVirtualChannelOpen(WTS_CURRENT_SERVER_HANDLE,dwSessionID,(char *)pChannelName);

	DWORD dwReadSize = 0;
	char* pVdQbuf = 0;
	BOOL bRet = WFAPI::pWFVirtualChannelQuery(hIcaChannel, WFVirtualClientData, (PVOID*)&pVdQbuf, &dwReadSize);
	m_log.FmtMessage(1, L"IcaChannelOpen - WFVirtualChannelOpen:[0x%08x] WFVirtualChannelQuery:(%d)[%d][0x%08x]"
		, hIcaChannel, bRet, dwReadSize, pVdQbuf);
	if( pVdQbuf)
	{
		m_log.DataMessage(2, L"WFVirtualChannelQuery", (LPBYTE)pVdQbuf, dwReadSize);
		WFAPI::pWFFreeMemory(pVdQbuf);
	}

	return hIcaChannel; 
}

BOOL CIcaChannelServer::IcaChannelClose(HANDLE m_hChannel)
{
	return WFAPI::pWFVirtualChannelClose(m_hChannel);
}


BOOL CIcaChannelServer::ChannelAsyncRead(ReadWritePacket *pPacket)
{
	BOOL         bReadSucess;

	bReadSucess = ReadFile(pPacket->hFile, pPacket->buff.buff, pPacket->buff.maxlength, 0, &pPacket->ol);
	if ( bReadSucess || GetLastError() == ERROR_IO_PENDING)
		return TRUE;

	return FALSE;

}
BOOL CIcaChannelServer::ChannelAsyncWrite(ReadWritePacket *pPacket)
{
	BOOL bWriteSucess = WriteFile(pPacket->hFile,pPacket->buff.buff, pPacket->buff.size, 0, &pPacket->ol);
	if ( bWriteSucess || GetLastError() == ERROR_IO_PENDING)
		return TRUE;

	return FALSE;
}

BOOL CIcaChannelServer::IcaChannelCanceIO(HANDLE m_hFile)
{
	return CancelIo(m_hFile);
}
