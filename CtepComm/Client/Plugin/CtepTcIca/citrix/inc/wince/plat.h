/*************************************************************************
 *
 *  wince\plat.h
 *
 *  WINCE specific platform types
 *
 *  Copyright (c) 1998 - 2000 Citrix Systems, Inc. All Rights Reserved.
 *
 ************************************************************************/
#ifndef _WINCE_PLAT_H_
#define _WINCE_PLAT_H_

#if defined(SHx) || defined(MIPS) || defined(ARM)
#define NEED_ALIGNMENT 1
#define ALIGNMENT_REQUIRED 1
#endif

#ifndef _MFC_VER
#include <windows.h>
#define LEGACY_TYPES_DEFINED
#endif

#define TW2_SCALE_TIMERS    /* Over-scrolling control */

/*
 * Basic platform types
 */
#define TYPEDEF_NATURAL int
#define TYPEDEF_8BITS   char
#define TYPEDEF_16BITS  short
#define TYPEDEF_32BITS  int

#define TYPEDEF_NEAR
#define TYPEDEF_FAR
#define TYPEDEF_CDECL     __cdecl
#define TYPEDEF_PASCAL    __stdcall
#define TYPEDEF_FASTCALL  __fastcall
#define TYPEDEF_LOADDS
#define TYPEDEF_INLINE    __inline

#define TYPEDEF_CONST   const

/*
 *  #pragma macros
 */
#define PLATFORM_PRAGMAS
#define ENG_PACKING_ENABLE  pack(8)
#define ENG_PACKING_RESTORE pack()

#define ICA_PACKING_ENABLE  pack(1)
#define ICA_PACKING_RESTORE pack()

/*
 * maximum path size
 */
#define PATH_MAX_SIZE   260


/*
 * Define linkage type for various components
 */
#define USE_AUBUF_CALL_TABLE
#define USE_AUD_CALL_TABLE
#define USE_BINI_CALL_TABLE
#define USE_CLIB_CALL_TABLE
#define USE_CTXOS_CALL_TABLE
#define USE_ENG_CALL_TABLE
#define USE_EVT_CALL_TABLE
#define USE_GRAPH_CALL_TABLE
#define USE_HND_CALL_TABLE
#define USE_INI_CALL_TABLE
#define USE_KBD_CALL_TABLE
#define USE_LIBMGR_CALL_TABLE
#define USE_LOG_CALL_TABLE
#define USE_MEM_CALL_TABLE
#define USE_MEMINI_CALL_TABLE
#define USE_MODULE_CALL_TABLE
#define USE_MOUSE_CALL_TABLE
#define USE_REDUCER_CALL_TABLE
#define USE_SYNC_CALL_TABLE
#define USE_THRD_CALL_TABLE
#define USE_TMR_CALL_TABLE
#define USE_VIO_CALL_TABLE
#define USE_VP_CALL_TABLE
#define USE_WND_CALL_TABLE
#define USE_SUBLST_CALL_TABLE
#define USE_SRCC_CALL_TABLE
#define USE_FILE_CALL_TABLE
#define USE_PATH_CALL_TABLE
#define USE_SCA_CALL_TABLE
#define USE_ASOCK_CALL_TABLE

/*
 * This define encapsulates code that relates to the speedbrowse project, for the client.
 */
#define SPEEDBROWSE

/* SleepEx defined */
#define SleepEx(x,y) Sleep(x)
#define ICA_SET_LED IcaChangeLed
#define Os_Delay Sleep

#include <platcall.h>

/*
 * Define the basic INT types that conflict with some platforms
 */

/* typedef signed TYPEDEF_NATURAL INT; defined by platform */

typedef signed TYPEDEF_8BITS   INT8;
typedef signed TYPEDEF_8BITS   TYPEDEF_NEAR *PINT8;
typedef signed TYPEDEF_8BITS   TYPEDEF_FAR  *LPINT8;
typedef signed TYPEDEF_16BITS  INT16;
typedef signed TYPEDEF_16BITS  TYPEDEF_NEAR *PINT16;
typedef signed TYPEDEF_16BITS  TYPEDEF_FAR  *LPINT16;
typedef signed TYPEDEF_32BITS  INT32;
typedef signed TYPEDEF_32BITS  TYPEDEF_NEAR *PINT32;
typedef signed TYPEDEF_32BITS  TYPEDEF_FAR  *LPINT32;

typedef unsigned TYPEDEF_NATURAL  *PUINT;
typedef unsigned TYPEDEF_NATURAL  TYPEDEF_FAR * LPUINT;
typedef unsigned TYPEDEF_8BITS    UINT8;
typedef unsigned TYPEDEF_8BITS    *PUINT8;
typedef unsigned TYPEDEF_8BITS    TYPEDEF_FAR * LPUINT8;
typedef unsigned TYPEDEF_16BITS   UINT16;
typedef unsigned TYPEDEF_16BITS   *PUINT16;
typedef unsigned TYPEDEF_16BITS    TYPEDEF_FAR * LPUINT16;
typedef unsigned TYPEDEF_32BITS   UINT32;
typedef unsigned TYPEDEF_32BITS   *PUINT32;
typedef unsigned TYPEDEF_32BITS    TYPEDEF_FAR * LPUINT32;

#ifdef TYPEDEF_64BITS
typedef TYPEDEF_64BITS   INT64;
typedef TYPEDEF_64BITS   *PINT64;
typedef unsigned TYPEDEF_64BITS    UINT64;
typedef unsigned TYPEDEF_64BITS    *PUINT64;
#endif

/* typedef TYPEDEF_8BITS CHAR; Defined by platform */
/* typedef TYPEDEF_16BITS        SHORT; Defined by platform */


/* VP character for basic VP strings */
typedef CHAR TYPEDEF_STRINGCHAR;


typedef INT (WINAPI *PDLLMAIN)( HINSTANCE, DWORD, LPVOID);

#include <platcomm.h>

#include <wcecalls.h>

#ifndef IN
#define IN
#endif /* IN */

#ifndef OUT
#define OUT
#endif /* OUT */

#ifndef INOUT
#define INOUT
#endif /* INOUT */

#include "platfix.h"

/*
 *----------------------------------------------------------------------------
 *  UNIFICATION : End
 *----------------------------------------------------------------------------
 */

#endif /* _WINCE_PLAT_H_ */
