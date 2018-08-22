/******************************************************************************
*
*  CTXDEBUG.H
*
*     Header file for Logging and Tracing ASSERT and TRACE macros
*
*  $Id: //icaclient/release/ica700/src/inc/ctxdebug.h#3 $
*  Copyright 1995-2003 Citrix Systems, Inc.  All Rights Reserved.
*
*******************************************************************************/

#ifndef __CTXDEBUG_H__
#define __CTXDEBUG_H__

/*=============================================================================
 ==   ASSERT macro
 ============================================================================*/
// #undef  ASSERT
// 
// #ifdef DEBUG
// #ifdef __GNUC__
// #define ASSERT(exp,rc) { \
//    if (!(exp)) { \
//       LogAssert( #exp, __FILE__, __LINE__, __FUNCTION__, rc ); \
//    } \
// }
// #else
// #define ASSERT(exp,rc) { \
//    if (!(exp)) { \
//       LogAssert( (PCHAR)#exp, (PCHAR)__FILE__, (int)__LINE__, (int)(rc) ); \
//    } \
// }
// #endif
// 
// #define Int3(a) LogPrintf(0xffffffff, 0xffffffff, a);
// 
// #else
// #define ASSERT(exp,rc) { }
// #define Int3(a) 
// #endif


/*=============================================================================
 ==   TRACE macros
 ============================================================================*/
#undef TRACE

#ifdef DEBUG
#define TRACE_ENABLED
#endif

#ifdef TRACE_ENABLED

#ifndef LOG_WHERE
#define LOG_WHERE
#endif

#define TRACE(_arg) {LOG_WHERE LogPrintf _arg;}
#undef TRACEBUF
#define TRACEBUF(_arg) {LOG_WHERE LogBuffer _arg;}
#define DEBUGBREAK() DebugBreak()
#else
#define TRACE(_arg) { }
#define TRACEBUF(_arg) { }
#define DEBUGBREAK() { }
#endif

/* Specifically defined to nothing in order to
 * be able to define away unwanted TRACE statements
 * by just changing TRACE to DTRACE */
#define DTRACE(_arg) { }
#define DTRACEBUF(_arg) { }
#define DASSERT(exp,rc) { }

/*=============================================================================
 ==   DeclareFName macro
 ==   NB. Used without a semicolon
 ============================================================================*/

/*  Debug Macro */
#ifdef DEBUG
#define DeclareFName(func) const char fName[] = func;
#else
#define DeclareFName(func)
#endif

#ifdef DOS32
void int3();
#pragma aux int3 = 0xCC;
#endif

#endif  /* __CTXDEBUG_H__ */
