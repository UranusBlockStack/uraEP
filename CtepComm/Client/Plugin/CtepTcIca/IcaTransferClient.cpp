#include "stdafx.h"

#include "CommonInclude/Tools/MoudlesAndPath.h"
#include "CommonInclude/RegEdit/RegEdit.h"

#include "../../CtepCommClient/CtepCommClientExport.h"

#include "IcaTransferClient.h"
#include "VirtualChannelManager.h"


#define CTEP_CLIENT_PATH	L"E:\\User\\Desktop\\CtepComm-ICA\\Debug"

typedef struct _VDPING_C2H
{
	VD_C2H Header;
}VDPING_C2H, *PVDPING_C2H;

extern "C"
{
	int __stdcall DriverOpen(PVD, PVDOPEN, PUINT16);
	int __stdcall DriverClose(PVD, PDLLCLOSE, PUINT16);
	int __stdcall DriverInfo(PVD, PDLLINFO, PUINT16);
	int __stdcall DriverQueryInformation(PVD, PVDQUERYINFORMATION, PUINT16);
	int __stdcall DriverSetInformation(PVD, PVDSETINFORMATION, PUINT16);
	int __stdcall DriverGetLastError(PVD, PVDLASTERROR);
	int __stdcall DriverPoll(PVD, PDLLPOLL, PUINT16);
	
};

//申明的全局变量
Log4CppLib					g_log("IcaTrans");

CIcaTransferClient *		g_pIcaTransClient = nullptr;
FnCTEPCommClientInitalize	g_fnInit;
FnCTEPCommClientClose		g_fnClose;


// 初始化CtepCommClient模组
BOOL InitializeCtepCommCltModule()
{
	static HMODULE hModule = nullptr;
	DWORD dwErr = 0;
	if ( !hModule)
	{
		CRegistry Reg(HKEY_LOCAL_MACHINE);
		// include in file:CtepComm/Client/CtepCommClient/DllRegisterCommClt.cpp
		static LPCWSTR CtepCommClientSubKey = L"Software\\CloudTimes\\CloudTimes Extension Protocol Client";
		static LPCWSTR CtepCommClientKeyName = L"Name";
		if ( !Reg.Open(CtepCommClientSubKey))
			goto ErrorEnd;

		CString Path;
		if ( !Reg.Read(CtepCommClientKeyName, Path))
			goto ErrorEnd;

		hModule = LoadLibrary(L"D:\\WorkSpace\\Project2\\CtepComm\\Debug\\CtepCommClient.dll");
		dwErr = GetLastError();
		g_log.FmtMessageW(3, L"InitializeCtepCommCltModule LoadLibrary path:[%s] Return Module:[0x%08x] Err:%d"
			, Path, hModule, dwErr);
		ASSERT(hModule);
	}

	if ( hModule)
	{
		HRESULT hr = E_FAIL;
		g_fnInit = GetFnCTEPCommClientInitalize(hModule);
		if ( !g_fnInit)
		{
			dwErr = GetLastError();
			g_log.FmtError(3, L"GetFnCTEPCommClientInitalize failed. err:%d", dwErr);
		}
		g_fnClose = GetFnCTEPCommClientClose(hModule);
		if ( !g_fnClose)
		{
			dwErr = GetLastError();
			g_log.FmtError(3, L"GetFnCTEPCommClientClose failed. err:%d", dwErr);
		}

		g_log.FmtMessage(1, L"g_fnInit[0x%08x] && g_fnClose[0x%08x]", g_fnInit, g_fnClose);
		ASSERT(g_fnInit && g_fnClose);
		if ( g_fnInit && g_fnClose)
		{
			if ( SUCCEEDED(g_fnInit(g_pIcaTransClient, 0, 0)))
				return TRUE;
		}
	}

ErrorEnd:
	ASSERT(0);
	return FALSE;
}


/*******************************************************************************
 *
 *  DriverOpen
 *
 *    Called once to set up things.
 *
 * ENTRY:
 *    pVd (input)
 *       pointer to virtual driver data structure
 *    pVdOpen (input/output)
 *       pointer to the structure VDOPEN
 *    puiSize (Output)
 *       size of VDOPEN structure.
 *
 * EXIT:
 *    CLIENT_STATUS_SUCCESS - no error
 *    CLIENT_ERROR_NO_MEMORY - could not allocate data buffer
 *    On other errors, an error code is returned from lower level functions.
 *
 ******************************************************************************/
int __stdcall DriverOpen(PVD pVd, PVDOPEN pVdOpen, PUINT16 puiSize)
{
	ASSERT(pVd);
	g_pIcaTransClient = new CIcaTransferClient(pVd);
	ASSERT(g_pIcaTransClient);
	g_pIcaTransClient->InitChannel();

	if ( !InitializeCtepCommCltModule())
	{
		ASSERT(0);
		delete g_pIcaTransClient;
		g_pIcaTransClient = nullptr;

		return CLIENT_ERROR;
	}

	*puiSize = sizeof(VDOPEN);
	pVdOpen->ChannelMask = g_pIcaTransClient->m_ChannelMask;

	g_log.FmtMessage(1, L"DriverOpen Out. ChannelMask:0x%08x", pVdOpen->ChannelMask);

	return CLIENT_STATUS_SUCCESS;
}


/*******************************************************************************
 *
 *  DriverClose
 *
 *  The user interface calls VdClose to close a Vd before unloading it.
 *
 * ENTRY:
 *    pVd (input)
 *       pointer to procotol driver data structure
 *    pVdClose (input/output)
 *       pointer to the structure DLLCLOSE
 *    puiSize (input)
 *       size of DLLCLOSE structure.
 *
 * EXIT:
 *    CLIENT_STATUS_SUCCESS - no error
 *
 ******************************************************************************/
int _stdcall DriverClose(PVD pVd, PDLLCLOSE pDllClose, PUINT16 puiSize)
{
	ASSERT(pVd == g_pIcaTransClient->m_pVd);
	//TRACE((TC_VD, TT_API1, "VDPING: DriverClose entered"));
	pVd->pPrivate = NULL;
	g_fnClose();
	g_pIcaTransClient->DriveClose();
	delete g_pIcaTransClient;
	return(CLIENT_STATUS_SUCCESS);
}


/*******************************************************************************
 *
 *  DriverInfo
 *
 *    This routine is called to get module information
 *
 * ENTRY:
 *    pVd (input)
 *       pointer to virtual driver data structure
 *    pVdInfo (output)
 *       pointer to the structure DLLINFO
 *    puiSize (output)
 *       size of DLLINFO structure
 *
 * EXIT:
 *    CLIENT_STATUS_SUCCESS - no error
 *
 ******************************************************************************/
int _stdcall DriverInfo(PVD pVd, PDLLINFO pDllInfo, PUINT16 puiSize)
{
	USHORT ByteCount;
	PVDPING_C2H pVdData;
	PMODULE_C2H pHeader;
	PVDFLOW pFlow;

	ByteCount = sizeof(VDPING_C2H);

	*puiSize = sizeof(DLLINFO);

	g_log.FmtMessageW(3, L"DriverInfo. ByteCount:%d", pDllInfo->ByteCount);
	// TRACE((TC_VD, TT_API1, "VDPING: DriverInfo entered"));

	// Check if buffer is big enough
	// If not, the caller is probably trying to determine the required
	// buffer size, so return it in ByteCount.
	if(pDllInfo->ByteCount < ByteCount)
	{
		pDllInfo->ByteCount = ByteCount;
		return(CLIENT_ERROR_BUFFER_TOO_SMALL);
	}

	// Initialize default data
	pDllInfo->ByteCount = ByteCount;
	pVdData = (PVDPING_C2H) pDllInfo->pBuffer;

	// Initialize module header

	pHeader = &pVdData->Header.Header;
	pHeader->ByteCount = ByteCount; 
	pHeader->ModuleClass = Module_VirtualDriver;

	pHeader->VersionL = 1;//CTXPING_VER_LO;
	pHeader->VersionH = 1;//CTXPING_VER_HI;

	strcpy_s((char*)(pHeader->HostModuleName), sizeof(pHeader->HostModuleName), "ICA"); // max 8 characters

	// Initialize virtual driver header
	pVdData->Header.ChannelMask = pVd->ChannelMask;
	pFlow = &pVdData->Header.Flow;
	pFlow->BandwidthQuota = 0;
	pFlow->Flow = VirtualFlow_None;

	// add our own data
	//pVdData->usMaxDataSize = 2047;//g_usMaxDataSize;

	pDllInfo->ByteCount = WIRE_WRITE(VDPING_C2H, pVdData, ByteCount);

	return(CLIENT_STATUS_SUCCESS);
}


/*******************************************************************************
 *
 *  DriverQueryInformation
 *
 *   Required vd function.
 *
 * ENTRY:
 *    pVd (input)
 *       pointer to virtual driver data structure
 *    pVdQueryInformation (input/output)
 *       pointer to the structure VDQUERYINFORMATION
 *    puiSize (output)
 *       size of VDQUERYINFORMATION structure
 *
 * EXIT:
 *    CLIENT_STATUS_SUCCESS - no error
 *
 ******************************************************************************/
int __stdcall DriverQueryInformation(PVD pVd, PVDQUERYINFORMATION pVdQueryInfo, PUINT16 puiSize)
{
	g_log.FmtMessageW(3, L"DriverQueryInformation. Class:%d",pVdQueryInfo->VdInformationClass);
	//TRACE(( TC_VD, TT_API1, "VDPING: DriverQueryInformation entered" ));
	//TRACE(( TC_VD, TT_API2, "pVdQueryInformation->VdInformationClass = %d", pVdQueryInformation->VdInformationClass));

	*puiSize = sizeof( VDQUERYINFORMATION );
	return( CLIENT_STATUS_SUCCESS );
	return 0;
}

/*******************************************************************************
 *
 *  DriverSetInformation
 *
 *   Required vd function.
 *
 * ENTRY:
 *    pVd (input)
 *       pointer to virtual driver data structure
 *    pVdSetInformation (input/output)
 *       pointer to the structure VDSETINFORMATION
 *    puiSize (input)
 *       size of VDSETINFORMATION structure
 *
 * EXIT:
 *    CLIENT_STATUS_SUCCESS - no error
 *
 ******************************************************************************/
int __stdcall DriverSetInformation(PVD pVd, PVDSETINFORMATION pVdSetInfo, PUINT16 puiSize)
{
	g_log.FmtMessageW(3, L"DriverSetInformation. Class:%d",pVdSetInfo->VdInformationClass);
	//TRACE((TC_VD, TT_API1, "VDPING: DriverSetInformation entered"));
	//TRACE((TC_VD, TT_API2, "pVdSetInformation->VdInformationClass = %d", pVdSetInformation->VdInformationClass));
	return( CLIENT_STATUS_SUCCESS );
}

/*******************************************************************************
 *
 *  DriverGetLastError
 *
 *   Queries error data.
 *   Required vd function.
 *
 * ENTRY:
 *    pVd (input)
 *       pointer to virtual driver data structure
 *    pLastError (output)
 *       pointer to the error structure to return (message is currently always
 *       NULL)
 *
 * EXIT:
 *    CLIENT_STATUS_SUCCESS - no error
 *
 ******************************************************************************/
int __stdcall DriverGetLastError(PVD pVd, PVDLASTERROR pVdError)
{
	//TRACE(( TC_VD, TT_API1, "VDPING: DriverGetLastError entered" ));
	/*
	*  Interpret last error and pass back code and string data
	*/

	pVdError->Error = pVd->LastError;
	pVdError->Message[0] = '\0';
	return(CLIENT_STATUS_SUCCESS);
}


/*******************************************************************************
 *
 *  DriverPoll
 *
 *  The Winstation driver calls DriverPoll
 *
 * ENTRY:
 *    pVd (input)
 *       pointer to virtual driver data structure
 *    pVdPoll (input)
 *       pointer to the structure DLLPOLL or DLL_HPC_POLL
 *    puiSize (input)
 *       size of DLLPOOL structure.
 *
 * EXIT:
 *    CLIENT_STATUS_SUCCESS - OK so far.  We will be polled again.
 *    CLIENT_STATUS_NO_DATA - No more data to send.
 *    CLIENT_STATUS_ERROR_RETRY - Could not send the data to the WD's output queue - no space.
 *                                Hopefully, space will be available next time.
 *    Otherwise, a fatal error code is returned from lower level functions.
 *
 *    NOTE:  CLIENT_STATUS_NO_DATA signals the WD that it is OK to stop
 *           polling until another host to client packet comes through.
 *
 * REMARKS:
 *    If polling is enabled (pre-HPC client, or HPC client with polling enabled),
 *    this function is called regularly.  DriverPoll is always called at least once.
 *    Otherwise (HPC with polling disabled), DriverPoll is called only when requested
 *    via WDSET_REQUESTPOLL or SENDDATA_NOTIFY.
 *
 ******************************************************************************/

/*  CLIENT_STATUS_SUCCESS   :	WD将定期轮询本channel
 *  CLIENT_STATUS_NO_DATA   :	WD可停止轮询本channel，直到本channel收到新的数据包
 *
 *  因等时传输，块传输的预读...可能本channel未收到数据包，但一直产生要发送的数据包，故我们必须永远返回CLIENT_STATUS_SUCCESS，   
 */
int __stdcall DriverPoll(PVD pVd, PDLLPOLL pVdPoll, PUINT16 puiSize)
{
	ASSERT(pVd == g_pIcaTransClient->m_pVd);
	return g_pIcaTransClient->IcaPollData(pVd, pVdPoll, puiSize);
}


/*******************************************************************************
 *
 *  ICADataArrival
 *
 *   A data PDU arrived over our channel.
 *
 * ENTRY:
 *    pVd (input)
 *       pointer to virtual driver data structure
 *
 *    uChan (input)
 *       ICA channel the data is for.
 *
 *    pBuf (input)
 *       Buffer with arriving data packet
 *
 *    Length (input)
 *       Length of the data packet
 *
 * EXIT:
 *       void
 *
 ******************************************************************************/
void WFCAPI ICADataArrival( PVOID pVd, USHORT uChan, LPBYTE pBuf, USHORT length)
{
	g_log.FmtMessageW(1, L"ICADataArrial - usChannel[%d] Buff[0x%08x],size[%d]", uChan, pBuf, length);

	ASSERT(g_pIcaTransClient && pVd == g_pIcaTransClient->m_pVd);
	if ( g_pIcaTransClient)
		g_pIcaTransClient->RecvData((char*)pBuf,length);
}


int RegisterVirtualChannel(PVD pVd,char* pChannelName,USHORT* pusChannelNum, ULONG* pulChannelMask)
{

	OPENVIRTUALCHANNEL OpenVirtualChannel = {0};
	WDQUERYINFORMATION wdqi;
	UINT16             uiSize;
	int                rc;

	wdqi.WdInformationClass = WdOpenVirtualChannel;
	wdqi.pWdInformation = &OpenVirtualChannel;
	wdqi.WdInformationLength = sizeof(OPENVIRTUALCHANNEL);
	OpenVirtualChannel.pVCName = pChannelName;

	uiSize = sizeof(WDQUERYINFORMATION);

	rc = VdCallWd(pVd, WDxQUERYINFORMATION, &wdqi, &uiSize);
	if( rc != CLIENT_STATUS_SUCCESS)
	{
		g_log.FmtError(3, L"RegisterVirtualChannel - VdCallWd. Could not open %s. rc=%d"
			, OpenVirtualChannel.pVCName, rc);
		return rc;
	}

	*pusChannelNum = OpenVirtualChannel.Channel;
	*pulChannelMask = (1L << OpenVirtualChannel.Channel);
	pVd->pPrivate = NULL;

	g_log.wprint(L"RegisterVirtualChannel. ChannelMask = 0x%08x,*pusChannelNum = %d"
		, *pulChannelMask, *pusChannelNum);

	return rc;
}

int RegisterWriteHook(PVD pVd,USHORT usChannelNum,VDWRITEHOOK* pRetVdwh)
{
	VDWRITEHOOK        vdwh;
	WDSETINFORMATION   wdsi;
	UINT16             uiSize;
	int                rc;

	vdwh.Type  = usChannelNum;
	vdwh.pVdData = pVd;
	vdwh.pProc = (PVDWRITEPROCEDURE)ICADataArrival;
	wdsi.WdInformationClass  = WdVirtualWriteHook;
	wdsi.pWdInformation      = &vdwh;
	wdsi.WdInformationLength = sizeof(VDWRITEHOOK);
	uiSize                   = sizeof(WDSETINFORMATION);

	rc = VdCallWd(pVd, WDxSETINFORMATION, &wdsi, &uiSize);
	ASSERT(rc == CLIENT_STATUS_SUCCESS);
	if(CLIENT_STATUS_SUCCESS != rc)
	{
		g_log.FmtError(3, L"RegisterWriteHook - VdCallWd. Could not register write hook. rc %d", rc);
		return(rc);
	}

	memcpy(pRetVdwh, &vdwh, sizeof(vdwh));
	return CLIENT_STATUS_SUCCESS;
}


