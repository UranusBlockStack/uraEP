/*************************************************************************
*
*  wince\platid.h
*
*  Platform specific types related to client identification
*
*  Copyright (c) 1998 - 2000 Citrix Systems, Inc. All Rights Reserved.
*
*************************************************************************/

#ifndef _WINCE_PLATID_H_
#define _WINCE_PLATID_H_

#define CLIENT_PRODUCT_ID       CLIENTID_CITRIX_WINCE

#ifdef _X86_
#define CLIENT_BASE_MODEL           CLIENT_MODEL_CTX_WINCE_X86
#endif
#ifdef _SH3_
#define CLIENT_BASE_MODEL           CLIENT_MODEL_CTX_WINCE_SH3
#endif
#ifdef _SH4_
#define CLIENT_BASE_MODEL           CLIENT_MODEL_CTX_WINCE_SH4
#endif
#ifdef _MIPS_
#define CLIENT_BASE_MODEL           CLIENT_MODEL_CTX_WINCE_MIPS
#endif
#ifdef _PPC_
#define CLIENT_BASE_MODEL           CLIENT_MODEL_CTX_WINCE_PPC
#endif
#ifdef _ARM_
#define CLIENT_BASE_MODEL           CLIENT_MODEL_CTX_WINCE_ARM
#endif
#ifndef CLIENT_BASE_MODEL
#error Unable to set CLIENT_BASE_MODEL: Platform not recognised.
#endif

/*
 *  DLL's used to identify the SecureICA model
 */
#define SECUREICA_40_BIT_DLL    "pdc40c.dll"
#define SECUREICA_56_BIT_DLL    "pdc56c.dll"
#define SECUREICA_128_BIT_DLL   "pdc128c.dll"

#endif /* _WINCE_PLATID_H_ */

