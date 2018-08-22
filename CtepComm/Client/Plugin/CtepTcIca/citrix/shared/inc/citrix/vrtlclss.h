/*++ BUILD Version: 0000 ******************************************************
*
*  VRTLCLSS.H
*     This file contains the virtual class enum, common to the server and client
*
*     NOTE: this file is shared between the client and the server
*
*  Copyright (c) 1997 - 2003 Citrix Systems, Inc. All Rights Reserved.
*
*  This is an unpublished work protected as such under copyright law,
*  containing proprietary and confidential information of Citrix Systems, Inc.
*  Use, disclosure or reproduction without the express written authorization
*  of Citrix Systems, Inc. is prohibited.
*
*******************************************************************************/

#ifndef __VRTLCLSS_H__
#define __VRTLCLSS_H__

/*
 *  Virtual i/o channel ids
 *  NOTE: don't change the order of this structure
 *  These are here for compatibility reasons (taken from ica-c2h.h)
 */

/* 
 * The if defined here is so we can determine if we are building on the server side
 * or the client side
 */
#if defined(WINFRAME_BUILD)

#if WINFRAME_BUILD
#define DEFINE_VC 1
#else
#define DEFINE_VC 0
#endif

#else
#define DEFINE_VC 1
#endif

#if DEFINE_VC
typedef enum _VIRTUALCLASS_COMPATIBILITY {
    Virtual_Screen    = 0,
    Virtual_LPT1      = 1,                /* also AUX1 */
    Virtual_LPT2      = 2,                /* also AUX2 */
    Virtual_Reserved3 = 3,
    Virtual_Cpm       = 4,                /* client print spooling */
    Virtual_COM1      = 5,
    Virtual_COM2      = 6,
    Virtual_Reserved4 = 7,
    Virtual_Ccm       = 8,                /* client COMM mapping */
    Virtual_ThinWire  = 9,                /* remote windows data */
    Virtual_Cdm       = 10,               /* client drive mapping */
    Virtual_PassThru  = 11,               /* passthrough for shadow */
    Virtual_User1     = 12,               /* user defined */
    Virtual_User2     = 13,               /* user defined */
    Virtual_User3     = 14,               /* user defined */
    Virtual_Reserved1 = 15,               /* reserved */
    Virtual_Reserved2 = 16,               /* reserved */
    Virtual_Clipboard = 17,               /* clipboard */
    Virtual_Maximum   = 32                /* TOTAL NUMBER OF VIRTUAL CHANNELS */
} VIRTUALCLASS__COMPATIBILITY;
#endif

/* 
 * The ifdef here is so we can determine if we are building on the server side
 * or the client side
 */
#ifdef WINFRAME_BUILD
#if WINFRAME_BUILD

#define Virtual_LPT4 Virtual_Cpm
#define Virtual_COM4 Virtual_Ccm
#define Virtual_LPT3 Virtual_Reserved3
#define Virtual_COM3 Virtual_Reserved4

typedef VIRTUALCLASS__COMPATIBILITY  _VIRTUALCLASS;
typedef VIRTUALCLASS__COMPATIBILITY   VIRTUALCLASS;
typedef VIRTUALCLASS__COMPATIBILITY *PVIRTUALCLASS;

#else /* winframe build */

typedef LONG VIRTUALCLASS;
typedef LONG * PVIRTUALCLASS;

#endif /* winframe build */

#else /* ifdef winframe_build */

typedef LONG VIRTUALCLASS;
typedef LONG * PVIRTUALCLASS;

#endif

#endif

