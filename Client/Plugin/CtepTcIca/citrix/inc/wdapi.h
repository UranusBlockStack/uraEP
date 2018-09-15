/*******************0000***********************************************
*
*  WDAPI.H
*
*  This module contains external winstation driver API
*  defines and structures
*
*  Copyright (c) 1994 - 2003 Citrix Systems, Inc. All Rights Reserved.
*
***********************************************************************/

#ifndef __WDAPI_H__
#define __WDAPI_H__

#if defined(PLATFORM_PRAGMAS) && defined(MKDBG)
#if defined(__DLLAPI_H__) && defined(_MINIDLL_H_)
#pragma MKDBG("DLLAPI and MINIDLL are included")
#elif defined(_MINIDLL_H_)
#pragma MKDBG("MINIDLL is included")
#elif defined(__DLLAPI_H__)
#pragma MKDBG("DLLAPI is included")
#endif
#endif /* PLATFORM_PRAGMAS && MKDBG */

#include <dllapi.h>
#include <vrtlclss.h>
#include <engtypes.h>
#include <ica.h>


/*
 *  Index into WD procedure array
 */

#define WDxQUERYINFORMATION      6
#define WDxSETINFORMATION        7
#define WDxCOUNT                 8  /* number of external wd procedures */

/*
 *  WdInformationClass enum
 */
typedef enum _WDINFOCLASS {
    WdClientData,
    WdStatistics,
    WdLastError,
    WdConnect,
    WdDisconnect,
    WdKillFocus,
    WdSetFocus,
    WdEnablePassThrough,
    WdDisablePassThrough,
    WdVdAddress,
    WdVirtualWriteHook,
    WdAddReadHook,
    WdRemoveReadHook,
    WdAddWriteHook,
    WdRemoveWriteHook,
    WdModemStatus,
    WdXferBufferSize,     /* Get: Size of host and client buffers */
    WdCharCode,
    WdScanCode,
    WdMouseInfo,
    WdInitWindow,
    WdDestroyWindow,
    WdRedraw,             /* Tell the host to redraw */
    WdThinwireStack,      /* Pass the thinwire stack to the thinwire vd */
    WdHostVersion,        /* get - Host Version information*/
    WdRealizePaletteFG,   /* inform client to realize it's foreground palette */
    WdRealizePaletteBG,   /* inform client to realize it's background palette */
    WdInactivate,         /* client is about to lose input focus */
    WdSetProductID,
    WdGetTerminateInfo,   /* test for legitimate terminate */
    WdRaiseSoftkey,       /* raise the soft keyboard */
    WdLowerSoftkey,       /* lower the soft keyboard */
    WdIOStatus,           /* IO status */
    WdOpenVirtualChannel, /* get - Open a virtual channel */
    WdCache,              /* persistent caching command set */
    WdGetInfoData,        /* Information from host to client */
    WdWindowSwitch,       /* window has switched back, check keyboard state */
#if defined(UNICODESUPPORT) || defined(USE_EUKS)
    WdUnicodeCode,        /* window has sent unicode information to wd */
#endif
#ifdef PACKET_KEYSYM_SUPPORT
    WdKeysymCode,         /* window has sent keysym information to wd */
#endif
#ifdef WIN32
    WdSetNetworkEvent,    /* This is for engine to pass a event handle down to TD if WinSock2 is suported. */
#endif
    WdPassThruLogoff,
    WdClientIdInfo,        /* Get the client identification information */
    WdPartialDisconnect,   /* A PN Refresh connection will disconnect */
    WdDesktopInfo,         /* Get/Set: information about the remote dekstop */
    WdSeamlessHostCommand, /* Set: Seamless Host Agent command */
    WdSeamlessQueryInformation, /* Get: Seamless information */
#ifdef UNICODESUPPORT
    WdZlRegisterUnicodeHook,      /* Set : Zero Latency Unicode hook registration*/
    WdZLUnRegisterUnicodeHook,     /* Set : Zero Latency Unicode hook unregistration*/
#endif
    WdZLRegisterScanCodeHook,    /* Set: Zero Latency scan code hook registration*/
    WdZlUnregisterScanCodeHook,  /* Set: Zero Latency scan code hook unregistration*/
    WdIcmQueryStatus,            /* Get: Ica Channel Monitoring status */
    WdIcmSendPingRequest,        /* Set: Send ping request and call callback if specified */
    WdSetCursor,                 /* Set: New cursor from TW. Data - PHGCURSOR */
    WdFullScreenMode,            /* Get: Return TRUE if full screen text is enabled */
    WdFullScreenPaint,           /* Set: Full Screen Mode needs redrawn*/
    WdSeamlessInfo,              /* Get: Seamless information/capabilities from WD */
    WdCodePage,                  /* Get: Retrieve server/client-default codepage */
    WdIcaControlCommand,         /* Set: ICA Control Command */
    WdReconnectInfo,             /* Get: Information needed for auto reconnect */
    WdServerSupportBWControl4Printing, /* return TRUE if server suports printer bandwidth control*/
    WdVirtualChannel,            /* Get: Virtual channel information */
    WdGetLatencyInformation,     /* Get: Latency information */
    WdKeyboardInput,             /* Get/Set: Enable/Disable keyboard input */
    WdMouseInput,                /* Get/Set: Enable/Disable mouse input */
    WdCredentialPassing,         /* Set: Passing Credentials through WD */
    WdRenderingMode,             /* Set: Virtual channel the rendering mode (Headless) */
    WdPauseResume,               /* Get/Set: Pause/Resume virtual channels */

} WDINFOCLASS;

/*
 *  WdQueryInformation structure
 */
typedef struct _WDQUERYINFORMATION {
    WDINFOCLASS WdInformationClass;
    LPVOID pWdInformation;
    USHORT WdInformationLength;
    USHORT WdReturnLength;
} WDQUERYINFORMATION, * PWDQUERYINFORMATION;

/*
 *  WdSetInformation structure
 */
typedef struct _WDSETINFORMATION {
    WDINFOCLASS WdInformationClass;
    LPVOID pWdInformation;
    USHORT WdInformationLength;
} WDSETINFORMATION, * PWDSETINFORMATION;
/*
 *  WdOpenVirtualChannel structure
 */
typedef struct _OPENVIRTUALCHANNEL {
    LPVOID  pVCName;
    USHORT  Channel;
} OPENVIRTUALCHANNEL, * POPENVIRTUALCHANNEL;

#define INVALID_VC_HANDLE 0xffff


/*
 *  Set Info Class enum
 */
typedef enum _SETINFOCLASS {
    CallbackComplete
} SETINFOCLASS, * PSETINFOCLASS;

/*
 *  Query Info Class enum
 */
typedef enum _QUERYINFOCLASS {
    QueryHostVersion,
    OpenVirtualChannel
} QUERYINFOCLASS, * PQUERYINFOCLASS;

/*
 *  Outbuf Buffer data structure
 */
typedef struct _OUTBUF {

    /*
     *  Non-inherited fields
     */
    struct _OUTBUF * pLink;      /* wd/td outbuf linked list */
    LPVOID pParam;               /* pointer to allocated parameter memory */
    LPBYTE pMemory;              /* pointer to allocated buffer memory */
    LPBYTE pBuffer;              /* pointer within buffer memory */
    USHORT MaxByteCount;         /* maximum buffer byte count (static) */
    USHORT ByteCount;            /* byte count pointed to by pBuffer */

    LPBYTE pSaveBuffer;          /* saved outbuf buffer pointer */
    USHORT SaveByteCount;        /* saved outbuf byte count */

    /*
     *  Inherited fields (when pd allocates new outbuf and copies the data)
     */
    ULONG StartTime;            /* pdreli - transmit time (used to measure roundtrip) */
#ifdef PLATFORM_PRAGMAS
    #pragma warning(disable : 4214)
#endif
    USHORT fControl: 1;         /* pdreli - control buffer (ack/nak) */
    USHORT fRetransmit: 1;      /* pdreli - outbuf retransmit */
    USHORT fCompress: 1;        /* pdcomp - compress data */
#ifdef PLATFORM_PRAGMAS
    #pragma warning(default : 4214)
#endif
    BYTE Sequence;              /* pdreli - sequence number */
    BYTE Fragment;              /* pdreli - fragment number */

} OUTBUF, * POUTBUF;

/*
 * WdIcmQueryStatus structure
 */
typedef struct _WDICMQUERYSTATUSDATA
{
   BOOL    fHostAvailable;  /* Host can support ICM    */
   UINT8   Version;         /* Agreed protocol version */

   BOOL    fPingEnabled;    /* The client can send ping packets      */
   BOOL    fBMEnabled;      /* Background channel monitoring enabled */

   UINT16  uBMRepeatDelay;  /* Background channel monitoring repeat delay, sec. */

   UINT32  LastLatency;
   UINT32  AverageLatency;

} WDICMQUERYSTATUSDATA, *PWDICMQUERYSTATUSDATA;

/*
 * WDSEAMLESSINFO structure
 */
typedef struct _WDSEAMLESSINFO
{
   BOOL     fSeamless20Logon;      /* The server can support seamless 2.0 logon */
   BOOL     fSynchronousSeamless;  /* MF20 synchronous seamless */

   BOOL     fEnabled;              /* Seamless mode enabled for this session */
   BOOL     fPaused;               /* Host agent paused (simulated full screen */

} WDSEAMLESSINFO, *PWDSEAMLESSINFO;

/*
 * WdIcmSendPingRequest structure
 */
typedef struct _WDICMSENDPINGREQUEST
{
   PVOID    pfnCallback;        /* Callback function. Can be NULL */
   PVOID    Context;            /* Callback context. Can be NULL  */
   BOOL     fUrgent;            /* if TRUE then flush on the next poll */

} WDICMSENDPINGREQUEST, *PWDICMSENDPINGREQUEST;

/*
 *  WdReconnectInfo structure
 */
typedef struct _WDRECONNECTINFO
{
   BOOL         bAddressValid; /* Name, Address, ClientName, and PortNum are valid */
   ADDRESS      ConnectionFriendlyName;
   ADDRESS      Name;
   ADDRESS      Address;
   ADDRESS      IntermediateAddress;
   CLIENTNAME   ClientName;
   USHORT       PortNum;
   ADDRESS      SSLGatewayName;
   USHORT       SSLGatewayPortNum;
   UINT16       cbCookie;   /* Input zero to query for cookie size  */
   PVOID        cookie[1];  /* Placeholder - cookie begins here in the buffer. 
                               This should always be the last field in this structure. 
                               In case this is'nt any field after this will be overwritten 
                               by the data in the cookie. */
} WDRECONNECTINFO, *PWDRECONNECTINFO;

typedef int  (PWFCAPI POUTBUFALLOCPROC)( LPVOID, POUTBUF * );
typedef void (PWFCAPI POUTBUFFREEPROC)( LPVOID, POUTBUF );
typedef int  (PWFCAPI PPROCESSINPUTPROC)( LPVOID, LPBYTE, USHORT );
typedef int  (PWFCAPI PSETINFOPROC)( LPVOID, SETINFOCLASS, LPBYTE, USHORT );
typedef void (PWFCAPI PIOHOOKPROC)( LPBYTE, USHORT );
typedef int  (PWFCAPI PQUERYINFOPROC)( LPVOID, QUERYINFOCLASS, LPBYTE, USHORT );
typedef int  (PWFCAPI POUTBUFRESERVEPROC)( LPVOID, USHORT );
typedef int  (PWFCAPI POUTBUFAPPENDPROC)( LPVOID, LPBYTE, USHORT );
typedef int  (PWFCAPI POUTBUFWRITEPROC)( LPVOID );
typedef int  (PWFCAPI PAPPENDVDHEADERPROC)( LPVOID, USHORT, USHORT );

#define FLUSH_IMMEDIATELY                   0

typedef struct _MEMORY_SECTION {
    UINT    length;
    LPBYTE  pSection;
} MEMORY_SECTION, far * LPMEMORY_SECTION;

typedef int (PWFCAPI PQUEUEVIRTUALWRITEPROC) (LPVOID, USHORT, LPMEMORY_SECTION, USHORT, USHORT);


/*=============================================================================
 ==   Virtual driver hook structures
 ============================================================================*/

/*
 *  Virtual driver write hook structure
 */
typedef void (PWFCAPI PVDWRITEPROCEDURE)( LPVOID, USHORT, LPBYTE, USHORT );

/*
 *  Used when registering virtual write hook  (WdVirtualWriteHook)
 */
typedef struct _VDWRITEHOOK {
    USHORT Type;                             /* in: virtual channel id */
    LPVOID pVdData;                          /* in: pointer to virtual driver data */
    PVDWRITEPROCEDURE pProc;                 /* in: pointer to vd write procedure (wd->vd) */
    LPVOID pWdData;                          /* out: pointer to wd structure */
    POUTBUFRESERVEPROC pOutBufReserveProc;   /* out: pointer to OutBufReserve */
    POUTBUFAPPENDPROC pOutBufAppendProc;     /* out: pointer to OutBufAppend */
    POUTBUFWRITEPROC pOutBufWriteProc;       /* out: pointer to OutBufWrite */
    PAPPENDVDHEADERPROC pAppendVdHeaderProc; /* out: pointer to AppendVdHeader */
    USHORT MaximumWriteSize;                 /* out: maximum ica write size */
    PQUEUEVIRTUALWRITEPROC pQueueVirtualWriteProc;  /* out: pointer to QueueVirtualWrite */
} VDWRITEHOOK, * PVDWRITEHOOK;

#endif /* __WDAPI_H__*/
