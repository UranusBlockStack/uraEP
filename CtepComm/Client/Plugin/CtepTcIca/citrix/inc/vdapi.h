/*******************0000****************************************************
*
*  VDAPI.H
*
*  This module contains external virtual driver API defines and structures
*
*  Copyright (c) 1994 - 2003 Citrix Systems, Inc. All Rights Reserved.
*
****************************************************************************/

#ifndef __VDAPI_H__
#define __VDAPI_H__

#include <icaconst.h>
#include <dllapi.h>
#include <ica.h>

/*
 *  Index into PD procedure array
 */
#define VDxQUERYINFORMATION      6
#define VDxSETINFORMATION        7
#define VDxCOUNT                 8   /* number of external procedures */


/*
 *  VdOpen structure
 */
typedef struct _VDOPEN {
    LPVOID pIniSection;
    PDLLLINK pWdLink;
    ULONG ChannelMask;      /* bit 0 = Virtual_Screen */
    PPLIBPROCEDURE pfnWFEngPoll;
    PPLIBPROCEDURE pfnStatusMsgProc;
    HND hICAEng;
} VDOPEN, FAR * PVDOPEN;


/*
 *  VdInformationClass enum
 *   additional information classes can be defined for a given
 *   VD, as long as the number used is greater than that generated
 *   by this enum list
 */

typedef enum _VDINFOCLASS {
    VdLastError,
    VdKillFocus,
    VdSetFocus,
    VdMousePosition,
    VdThinWireCache,
    VdWinCEClipboardCheck,    /*  Used by WinCE to check for clipboard changes */
    VdDisableModule,
    VdFlush,
    VdInitWindow,
    VdDestroyWindow,
    VdPaint,
    VdThinwireStack,
    VdRealizePaletteFG,   /* inform client to realize it's foreground palette */
    VdRealizePaletteBG,   /* inform client to realize it's background palette */
    VdInactivate,         /* client is about to lose input focus */
    VdGetSecurityAccess,  /* cdm security info */
    VdSetSecurityAccess,  /* cdm security info */
    VdSendLogoff,
    VdCCShutdown,

    VdSeamlessHostCommand,        /* Seamless command call */
    VdSeamlessQueryInformation,   /* Seamless query call */

    VdWindowSwitch,
    VdSetCursor,                  /* Set: New cursor from TW. Data - PHGCURSOR */
    VdSetCursorPos,               /* Set: New cursor position. Data - VPPOINT */

    VdEnableState,                /* Set/Get driver state (enabled/disabled) */

    VdIcaControlCommand,

    VdVirtualChannel,             /* Set/Get virtual channel data */
    VdWorkArea,                   /* Set the work area of the application */
    VdSupportHighThroughput,
    VdRenderingMode,              /* Set/Get the rendering mode (headless client) */
    VdPauseResume,                /* Pause/Resume commands                        */

} VDINFOCLASS;

/*
 *  VdQueryInformation structure
 */
typedef struct _VDQUERYINFORMATION {
    VDINFOCLASS VdInformationClass;
    LPVOID pVdInformation;
    int VdInformationLength;
    int VdReturnLength;
} VDQUERYINFORMATION, * PVDQUERYINFORMATION;

/*
 *  VdSetInformation structure
 */
typedef struct _VDSETINFORMATION {
    VDINFOCLASS VdInformationClass;
    LPVOID pVdInformation;
    int VdInformationLength;
} VDSETINFORMATION, * PVDSETINFORMATION;

/*
 *  VdLastError
 */
typedef struct _VDLASTERROR {
    int Error;
    char Message[ MAX_ERRORMESSAGE ];
} VDLASTERROR, * PVDLASTERROR;

/*
 * VD Flush
 */
typedef struct _VDFLUSH {
    UCHAR Channel;
    UCHAR Mask;
} VDFLUSH, * PVDFLUSH;

#define  VDFLUSH_MASK_PURGEINPUT    0x01
#define  VDFLUSH_MASK_PURGEOUTPUT   0x02

#endif /* __VDAPI_H__ */
