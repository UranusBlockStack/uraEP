
/***************************************************************************
*
*  VD.H
*
*  This module contains Virtual Driver (PD) defines and structures
*
*  Copyright (c) 1994 - 2003 Citrix Systems, Inc. All Rights Reserved.
*
****************************************************************************/

#ifndef __VD_H__
#define __VD_H__

#include <dllapi.h>

/*=============================================================================
==   typedefs
=============================================================================*/

typedef struct _VD * PVD;

/*
 *  VD structure
 */
typedef struct _VD {

    ULONG ChannelMask;                  /* bit mask of handled channels */
    PDLLLINK pWdLink;                   /* pointer to winstation driver */
    int LastError;                      /* Last Error code */
    PVOID pPrivate;                     /* pointer to VD uncommon data */

} VD;

/*=============================================================================
==   function prototypes
=============================================================================*/
extern  "C" int  __stdcall VdCallWd( PVD pVd, USHORT ProcIndex, PVOID pParam, PUINT16 puiSize );
extern PLIBMGRTABLE VdLibMgr();


#endif /*__VD_H__*/

