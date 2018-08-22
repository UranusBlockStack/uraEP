/*************************************************************************
*
*  wince\platsys.h
*
*  Copyright (c) 1998 - 2000 Citrix Systems, Inc. All Rights Reserved.
*
*************************************************************************/

#ifndef _WINCE_PLATSYS_H_
#define _WINCE_PLATSYS_H_

/*
 * Platform specfic prototypes
 */

/*
 * Platform specific macros
 */

#define CtxCharUpper        CharUpperA
#define HCTXGLOBAL          HGLOBAL
/*
 *  Note WinCE doesn't support early completion for I/O.
 */
#define CtxYield()   Sleep(25)

#endif /* _WINCE_PLATSYS_H_ */

