/*************************************************************************
 *
 *  platfix.h
 *
 *  Fixups that allow the WinCE client to build with new common include
 *  files that are used.
 *
 * Copyright 2001 Citrix Systems Inc.  All Rights Reserved
 *
 ************************************************************************/
#ifndef _WINCE_PLATFIX_H_  
#define _WINCE_PLATFIX_H_

/* see mmsystem.h */
typedef UINT MMRESULT;

#define WFENG_NUM_HOTKEYS     8

/* Disable 'anachronism used: modifiers on data are ignored' warning message */
#pragma warning(disable:4229)

#endif /* _WINCE_PLATFIX_H_ */

