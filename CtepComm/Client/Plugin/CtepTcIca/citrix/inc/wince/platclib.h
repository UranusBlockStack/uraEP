/*************************************************************************
*
* wince\platclib.h
*
* WINCE specific Header file for C runtime library routines 
*
* Copyright (c) 1990 - 2000 Citrix Systems, Inc. All Rights Reserved.
*
*************************************************************************/
#ifndef _PLATCLIB_H_
#define _PLATCLIB_H_

/* FILE is defined in STDLIB.H for Windows CE 2.1 and above */

#if _WIN32_WCE < 210
typedef struct _iobuf FILE;
#endif 

#include <clibcomm.h>

#define memicmp    _memicmp
#define stricmp    _stricmp
#define strdup     _strdup
#define strnicmp   _strnicmp

#define access          _access
#define mkdir           _mkdir
#define rmdir           _rmdir
#define open            _open
#define close           _close
#define read            _read
#define lseek           _lseek
#define write           _write
#define remove          _remove


#ifndef _remove
int __cdecl _remove(const char * name);
#endif
#ifndef _access
int __cdecl _access(const char *path, int mode);
#endif


//#define GetLastErrno()  errno

//#define       rename(from, to) MoveFile(from, to)

#endif /* _PLATCLIB_H_ */

