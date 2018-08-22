/***************************************************************************
*
*   MIAPI.H
*
*   Header file for Memory INI APIs
*
*   Copyright (c) 1997 - 2003 Citrix Systems, Inc. All Rights Reserved.
*
****************************************************************************/

#ifndef __MIAPI_H__
#define __MIAPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_MEMINI_STR_LEN    40
#define MAXLINEBUF           128

typedef int (PWFCAPI PMISETPROFILEPOINTER)( PCHAR );
typedef int (PWFCAPI PMISETSECTIONPOINTER)( PCHAR );
typedef int (PWFCAPI PMIGETPRIVATEPROFILESTRING)( PCHAR, PCHAR, PCHAR,
                                                  PCHAR, int);
typedef int (PWFCAPI PMIGETPRIVATEPROFILEINT)( PCHAR, PCHAR, int);
typedef LONG (PWFCAPI PMIGETPRIVATEPROFILELONG)( PCHAR, PCHAR, long);
typedef int (PWFCAPI PMIGETPRIVATEPROFILEBOOL)( PCHAR, PCHAR, BOOL);
typedef int (PWFCAPI PMIRELEASEPROFILE)( );

typedef struct _MEMINITABLE
{
    PMISETPROFILEPOINTER        pmiSetProfilePointer;
    PMISETSECTIONPOINTER        pmiSetSectionPointer;
    PMIGETPRIVATEPROFILESTRING  pmiGetPrivateProfileString;
    PMIGETPRIVATEPROFILEINT     pmiGetPrivateProfileInt;
    PMIGETPRIVATEPROFILELONG    pmiGetPrivateProfileLong;
    PMIGETPRIVATEPROFILEBOOL    pmiGetPrivateProfileBool;
    PMIRELEASEPROFILE           pmiReleaseProfile;
} MEMINITABLE, FAR * PMEMINITABLE, FAR * FAR * PPMEMINITABLE;
/*=============================================================================
==   External functions provided by LoadLibraries()
=============================================================================*/

int WFCAPI MemIniLoad (PPMEMINITABLE );
int WFCAPI MemIniUnload( VOID );

/*=============================================================================
==   API entry points
=============================================================================*/

#define MEMINI_STRING "CTXMEMINI"

#ifdef USE_MEMINI_CALL_TABLE
#if defined(PLATFORM_PRAGMAS) && defined(MKVERB)
#pragma MKVERB (__FILE__": MEMINI_CALL_TABLE defined")
#endif

#define MEMINI_GLOBAL_INIT PMEMINITABLE g_pMemIniCallTable = NULL
#define MEMINI_EXTERN_INIT extern PMEMINITABLE g_pMemIniCallTable

#define MemIniRunTimeInit(pLibMgr, pVer) \
    ((*pLibMgr->pLMgrUse)(MEMINI_STRING,pVer,(PPLMGRTABLE)&g_pMemIniCallTable))
#define MemIniRunTimeRelease(pLibMgr) \
    ((*pLibMgr->pLMgrRelease)((PLMGTABLE)g_pMemIniCallTable))

MEMINI_EXTERN_INIT;/* compiling the library itself */

/* compiling someone else who uses the library */

#define miSetProfilePointer (*(g_pMemIniCallTable)->pmiSetProfilePointer)
#define miSetSectionPointer (*(g_pMemIniCallTable)->pmiSetSectionPointer)
#define miGetPrivateProfileString   \
                        (*(g_pMemIniCallTable)->pmiGetPrivateProfileString)
#define miGetPrivateProfileInt      \
                        (*(g_pMemIniCallTable)->pmiGetPrivateProfileInt)
#define miGetPrivateProfileLong     \
                        (*(g_pMemIniCallTable)->pmiGetPrivateProfileLong)
#define miGetPrivateProfileBool     \
                        (*(g_pMemIniCallTable)->pmiGetPrivateProfileBool)
#define miReleaseProfile            \
                        (*(g_pMemIniCallTable)->pmiReleaseProfile)

#else /* !USER_MEMINI_CALL_TABLE */

/* compiling the library itself */
int WFCAPI miSetProfilePointer(PCHAR);
int WFCAPI miSetSectionPointer(PCHAR);
int WFCAPI miGetPrivateProfileString( PCHAR, PCHAR, PCHAR,PCHAR, int);
int WFCAPI miGetPrivateProfileInt(PCHAR,PCHAR,int);
LONG WFCAPI miGetPrivateProfileLong(PCHAR,PCHAR,long);
int WFCAPI miGetPrivateProfileBool(PCHAR,PCHAR,BOOL);
int WFCAPI miReleaseProfile();

#define MEMINI_GLOBAL_INIT
#define MEMINI_EXTERN_INIT

#define MemIniRunTimeInit(pLibMgr, pVer)
#define MemIniRunTimeRelease(pLibMgr)

#endif  /* !USE_MEMINI_CALL_TABLE */

#ifdef __cplusplus
}
#endif

#endif /* __MIAPI_H__ */
