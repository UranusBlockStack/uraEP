/***************************************************************************
*
*  DLLAPI.H
*
*  This module contains client DLL access definitions
*
*  Copyright 1994-2003 Citrix Systems, Inc.  All Rights Reserved
*
****************************************************************************/

#ifndef __DLLAPI_H__
#define __DLLAPI_H__

#include <libmgr.h>

#if defined(PLATFORM_PRAGMAS) && defined(MKDBG)
#if defined(_MINIDLL_H_)
#pragma MKDBG("MINIDLL is included prior to DLLAPI")
#endif
#endif /* PLATFORM_PRAGMAS && MKDBG */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _EMBEDDLL FAR *PEMBEDDLL;
/*
 *  DLL link structure
 */
typedef struct _DLLLINK {
/* NOTE: 1st six elements must be same as MINIDLL */
    USHORT Segment;             /* starting seg of mem allocated for dll (aligned) */
    USHORT DllSize;             /* actual size of dll in paragraphs */
    USHORT ProcCount;           /* number of procedures in call table */
    PVOID pProcedures;          /* pointer to procedure call table */
    PVOID pData;                /* pointer to dll data structure */
    PUCHAR pMemory;             /* pointer to malloced memory (not aligned) */
    BYTE ModuleName[13];        /* client module name (8.3) */
    LPVOID pLibMgrCallTable;    /* Pointer to LibMgrTable (input) */
    USHORT ModuleDate;          /* module date in dos format */
    USHORT ModuleTime;          /* module time in dos format */
    ULONG ModuleSize;           /* module file size in bytes */
    struct _DLLLINK * pNext;    /* pointer to next DLLLINK structure */
    ULONG DllFlags;             /* DLL flags (embed..) */
    /* Everything after here is not included for the ModuleEnum call. */

    HND		LibraryHandle;

} DLLLINK, * PDLLLINK;

typedef int (PWFCAPI PMAINLOAD)( PDLLLINK );
#define ENUM_DLLLINK_SIZE (sizeof(DLLLINK))  

/*
 * Dll flags definition (32 of them possible)
 */
#define DLL_FLAGS_EMBED    1

typedef int (PWFCAPI PLOADPROCEDURE)( PDLLLINK, USHORT );
typedef int (PWFCAPI PDLLPROCEDURE)( PVOID, PVOID, PUINT16 );
#define MODULExLOAD            0
#define MODULExUNLOAD          1
#define MODULExENUM            2
#define MODULExCALL            3

 

typedef int (PWFCAPI PMODULELOAD)( char *, PDLLLINK );
typedef int (PWFCAPI PMODULEUNLOAD)( PDLLLINK );
typedef int (PWFCAPI PMODULEENUM)( LPBYTE, USHORT, PUSHORT );
typedef int (PWFCAPI PMODULECALL)( PDLLLINK, USHORT, PVOID, PUINT16 );

typedef struct _MODULETABLE
{
    PMODULELOAD     pModuleLoad;
    PMODULEUNLOAD   pModuleUnload;
    PMODULEENUM     pModuleEnum;
    PMODULECALL     pModuleCall;
}MODULETABLE,  * PMODULETABLE,  *  * PPMODULETABLE;

int WFCAPI ModuleInit(  PDLLLINK, PPMODULETABLE,PMAINLOAD,PEMBEDDLL );
int WFCAPI ModuleUninit( );


#define MODULE_STRING "CTXMODULE"

#ifdef USE_MODULE_CALL_TABLE
#if defined(PLATFORM_PRAGMAS) && defined(MKVERB)
#pragma MKVERB (__FILE__": MODULE_CALL_TABLE defined")
#endif

/*
 * Compiling the library itself
 */

#define MODULE_GLOBAL_INIT PMODULETABLE g_pModuleCallTable =NULL
#define MODULE_EXTERN_INIT extern PMODULETABLE g_pModuleCallTable

#define ModuleRunTimeInit(pLibMgr, pVer) \
    ((*pLibMgr->pLMgrUse)(MODULE_STRING,pVer,(PPLMGRTABLE)&g_pModuleCallTable))
#define ModuleRunTimeRelease(pLibMgr) \
    ((*pLibMgr->pLMgrRelease)((PLMGTABLE)g_pModuleCallTable))

MODULE_EXTERN_INIT;

/*
 * Note: These function typedefs must be maintained in sync with the
 *       above function prototypes
 */


#define ModuleLoad     (*(g_pModuleCallTable)->pModuleLoad)
#define ModuleUnload   (*(g_pModuleCallTable)->pModuleUnload)
#define ModuleEnum     (*(g_pModuleCallTable)->pModuleEnum)
#define ModuleCall     (*(g_pModuleCallTable)->pModuleCall)

#else /* !USE_MODULE_CALL_TABLE */

/*
 * Note: These function prototypes must be maintained in sync with the
 *       function typedefs below
 */
int WFCAPI ModuleLoad( char *, PDLLLINK );
int WFCAPI ModuleUnload( PDLLLINK );
int WFCAPI ModuleEnum( LPBYTE, USHORT, PUSHORT );
int WFCAPI ModuleCall( PDLLLINK, USHORT, PVOID, PUINT16 );

#define MODULE_GLOBAL_INIT
#define MODULE_EXTERN_INIT

#define ModuleRunTimeInit(pLibMgr, pVer)
#define ModuleRunTimeRelease(pLibMgr)

#endif /* !USE_MODULE_CALL_TABLE */

/*
 *  ExeOpen structure
 */
typedef struct _EXEOPEN {
    PVOID pIniSection;
} EXEOPEN, * PEXEOPEN;


/*=============================================================================
 ==   Common dll entry points for all dlls
 ============================================================================*/
 

#define DLLxLOAD                 0
#define DLLxUNLOAD               1
#define DLLxOPEN                 2
#define DLLxCLOSE                3
#define DLLxINFO                 4
#define DLLxPOLL                 5

/*
 *  DLLxCLOSE structure
 */
typedef struct _DLLCLOSE {
    int NotUsed;
} DLLCLOSE, * PDLLCLOSE;

/*
 *  DLLxINFO structure
 */
typedef struct _DLLINFO {
    LPBYTE pBuffer;
    USHORT ByteCount;
} DLLINFO, * PDLLINFO;

/*
 *  DLLxPOLL structure
 */
typedef struct _DLLPOLL {
    ULONG CurrentTime;          /* current time in msec */
} DLLPOLL, * PDLLPOLL;

typedef INT (PWFCAPI PEMBEDLOAD)( PDLLLINK pLink );

typedef struct _EMBEDDLL {
  char      *DllName;                  /* name of DLL */
  PEMBEDLOAD DllLoad;                  /* DLL load function */
  PVOID      pfnMain;                  /* DLL LIBMAIN or DLLMain entry */
} EMBEDDLL;


#ifdef __cplusplus
}
#endif

#endif /* __DLLAPI_H__ */
