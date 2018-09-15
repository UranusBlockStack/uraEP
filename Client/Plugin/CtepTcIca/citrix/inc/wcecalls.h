/******************************************************************************
*
*  WCECALLS.H
*
*  Support for ANSI versions of Win32 files.
*  Windows CE has no ANSI support so we will add
*  all needed APIs here.
*
*  Copyright 1995-2003 Citrix Systems, Inc.  All Rights Reserved
*
*******************************************************************************/

#ifdef WINCE
#ifndef _WCECALLS_H_
#define _WCECALLS_H_
#include <wcetrace.h>

/* We don't want wcecalls to remap for unicode files, except for BRAPI */
/* where UNICODE is defined, together with CLIENT */
#if !defined(UNICODE) || defined(CLIENT)

/* This is the CITRIXCE.C section for WCECALLS.LIB */
#ifndef __CITRIX_CE__
#define __CITRIX_CE__
#ifdef WINCE

/* Included for remapped
 * winsock calls.
 */
#include <winsock.h>

#ifdef __cplusplus
extern "C" {
#endif


#define MAX_COMPUTERNAME_LENGTH 15
#if _WIN32_WCE < 300
typedef struct tagNONCLIENTMETRICSW
{
    UINT    cbSize;
    int     iBorderWidth;
    int     iScrollWidth;
    int     iScrollHeight;
    int     iCaptionWidth;
    int     iCaptionHeight;
    LOGFONTW lfCaptionFont;
    int     iSmCaptionWidth;
    int     iSmCaptionHeight;
    LOGFONTW lfSmCaptionFont;
    int     iMenuWidth;
    int     iMenuHeight;
    LOGFONTW lfMenuFont;
    LOGFONTW lfStatusFont;
    LOGFONTW lfMessageFont;
}   NONCLIENTMETRICSW, *PNONCLIENTMETRICSW, FAR* LPNONCLIENTMETRICSW;
typedef NONCLIENTMETRICSW NONCLIENTMETRICS;
typedef PNONCLIENTMETRICSW PNONCLIENTMETRICS;
typedef LPNONCLIENTMETRICSW LPNONCLIENTMETRICS;
typedef struct tagMINIMIZEDMETRICS
{
    UINT    cbSize;
    int     iWidth;
    int     iHorzGap;
    int     iVertGap;
    int     iArrange;
}   MINIMIZEDMETRICS, *PMINIMIZEDMETRICS, *LPMINIMIZEDMETRICS;
typedef struct tagICONMETRICSW
{
    UINT    cbSize;
    int     iHorzSpacing;
    int     iVertSpacing;
    int     iTitleWrap;
    LOGFONTW lfFont;
}   ICONMETRICSW, *PICONMETRICSW, *LPICONMETRICSW;
typedef ICONMETRICSW ICONMETRICS;
typedef PICONMETRICSW PICONMETRICS;
typedef LPICONMETRICSW LPICONMETRICS;
typedef struct tagWINDOWPLACEMENT {
    UINT  length;
    UINT  flags;
    UINT  showCmd;
    POINT ptMinPosition;
    POINT ptMaxPosition;
    RECT  rcNormalPosition;
} WINDOWPLACEMENT;
typedef WINDOWPLACEMENT *PWINDOWPLACEMENT, *LPWINDOWPLACEMENT;
#endif

#define WPF_SETMINPOSITION      0x0001
#define WPF_RESTORETOMAXIMIZED  0x0002


/*
 * Global allocations are rewritten.
 * See .c file for details.
 */


#if _WIN32_WCE < 300
#ifndef GMEM_FIXED
/* Remap global heap function
    * names to avoid conflict with
    * emulator libraries.
    */
#define GlobalAlloc     GlobalAllocCE
#define GlobalReAlloc   GlobalReAllocCE
#define GlobalLock      GlobalLockCE
#define GlobalUnlock    GlobalUnlockCE
#define GlobalFree      GlobalFreeCE
#define GlobalSize      GlobalSizeCE

   /* Ignore flags.
    * Memory will always be fixed and zero inited.
    */
#define GMEM_FIXED          0
#define GMEM_MOVEABLE       0
#define GMEM_ZEROINIT       0
#define GHND                0
#define GPTR                0

HGLOBAL WINAPI GlobalAlloc(UINT uFlags, DWORD dwBytes);
HGLOBAL WINAPI GlobalReAlloc(HGLOBAL hMem, DWORD dwBytes, UINT uFlags);
LPVOID  WINAPI GlobalLock(HGLOBAL hMem);
BOOL  WINAPI GlobalUnlock(HGLOBAL hMem);
HGLOBAL WINAPI GlobalFree(HGLOBAL hMem);
DWORD WINAPI GlobalSize(HGLOBAL hMem);
#endif
#endif

/*
 * Constants used by the old-fashioned _open() command. Faked for WinCE.
 */

#ifndef O_RDWR
#define O_RDONLY       0x0000  /* open for reading only */
#define O_WRONLY       0x0001  /* open for writing only */
#define O_RDWR         0x0002  /* open for reading and writing */
#define O_APPEND       0x0008  /* writes done at eof */

#define O_CREAT        0x0100  /* create and open file */
#define O_TRUNC        0x0200  /* open and truncate */
#define O_EXCL         0x0400  /* open only if file doesn't already exist */
#endif

/*
 * ===========================================================================
 *  Following block copied from WINUSER.H.
 * BUGBUGCE ADU
 * We will have to emulate this messages
 * with a left mouse + modifier.
 */

#define WM_RBUTTONDOWN                  0x0204
#define WM_RBUTTONUP                    0x0205
#define WM_RBUTTONDBLCLK                0x0206
#define WM_MBUTTONDOWN                  0x0207
#define WM_MBUTTONUP                    0x0208
#define WM_MBUTTONDBLCLK                0x0209


/*
 * ===========================================================================
 *  Following block copied from WINUSER.H.  These values aren't defined in
 *  WinCE, but we need them in order to compile SHELLSUB.  It's fine to put
 *  this in, because the code is just checking a value sent by the host for
 *  rejecting Clipboard Formats that we don't understand.
 */

#define CF_METAFILEPICT     3
#define CF_ENHMETAFILE      14
#define CF_OWNERDISPLAY     0x0080
#define CF_PRIVATEFIRST     0x0200
#define CF_PRIVATELAST      0x02FF


/*
 * ===========================================================================
 *  These constants are not in the WinCE 2.0 beta header files, but
 * should be before WinCE 2.0 ships.
 *
 * BUGBUGCE
 */

   /* The Win CE 2.0 doc says there should
    * be a stock object for OEM Fixed Fonts,
    * but it is not in the header.
    */
#define OEM_FIXED_FONT      10


/*
 * ===========================================================================
 *  Mappings from Win32 functions to their Ext equivalents.
 * WinCE only supports the latest version.
 */

#define TextOut(hdc, x, y, lpString, cbString) \
   ExtTextOut(hdc, x, y, 0, NULL, lpString, cbString, NULL)

#define Beep(dwFreq, dwDuration) CEBeep(dwFreq, dwDuration)

   /* ScrollWindow() not supported.
    * Map to ScrollWindowEx()
    */
   BOOL ScrollWindow(HWND hWnd, int XAmount, int YAmount, CONST RECT *lpRect,
                  CONST RECT *lpClipRect);


   /* FileTimeToDosDateTime() not supported */
   BOOL
   WINAPI
   FileTimeToDosDateTime(
       CONST FILETIME *lpFileTime,
       UNALIGNED LPWORD lpFatDate,
       UNALIGNED LPWORD lpFatTime
       );

   /* DosDateTimeToFileTime() not supported */
   BOOL
   WINAPI
   DosDateTimeToFileTime(
       WORD wFatDate,
       WORD wFatTime,
       LPFILETIME lpFileTime
       );

   /* WinCE windows are never Iconic. */
#define IsIconic( hWnd ) FALSE

   /* BUGBUGCE ADU
    * This will be replaced when we
    * do cursor management.
    */
#define ClipCursor(i)

/* Stubs to be removed later */
#define GlobalAddAtom     GlobalAddAtomCE
#define GlobalGetAtomName GlobalGetAtomNameCE
#define GlobalDeleteAtom  GlobalDeleteAtomCE
#define DuplicateHandle   DuplicateHandleCE
ATOM
WINAPI
GlobalAddAtom(
    LPCSTR lpString
    );
UINT
WINAPI
GlobalGetAtomName(
    ATOM nAtom,
    LPSTR lpBuffer,
    int nSize
    );
ATOM
WINAPI
GlobalDeleteAtom(
    ATOM nAtom
    );
BOOL
WINAPI
DuplicateHandle(
    HANDLE hSourceProcessHandle,
    HANDLE hSourceHandle,
    HANDLE hTargetProcessHandle,
    LPHANDLE lpTargetHandle,
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwOptions
    );


/*
 * ===========================================================================
 *  StdLib redefinitions.
 * Macros and routines which Microsoft left out for WinCE
 * String.h is not in here since we are using the string library
 * from VC++ 5.0 and not the one from the CE
 */

#if _WIN32_WCE < 300
#define toupper toupperce
char toupper(char a);
#define tolower tolowerce
char tolower(char a);
#define isupper isupperce
int isupper(char c);
#endif


   /* Map Win32 calls to stdlib.
    * In the next block they are mapped
    * to citrix ce.
    */
#undef   lstrcat
#define  lstrcat     strcat
#undef   lstrcpy
#define  lstrcpy     strcpy
#undef   lstrcpyn
#define  lstrcpyn    strncpy
#undef   wsprintf
#define  wsprintf    sprintf
#undef   lstrlen
#define  lstrlen     strlen
#undef   wvsprintf
#define  wvsprintf   vsprintf
#undef   lstrcmpi
#define  lstrcmpi    _stricmp

   /* Copied from stdlib source.
    * These functions are not multibyte enabled.
    */
#undef  _strdup
#define _strdup   _strdupce
#undef   strspn
#define   strspn      strspnce
#undef   _stricmp
#define  _stricmp    _stricmpce
#undef   _strnicmp
#define  _strnicmp   _strnicmpce
#undef    strrchr
#define   strrchr     strrchrce
#undef    strpbrk
#define   strpbrk     strpbrkce
#undef    strcat
#define   strcat      strcatce
#undef    strcpy
#define   strcpy      strcpyce
#undef    sprintf
#define   sprintf     sprintfce
#undef   _vsnprintf
#define  _vsnprintf  _vsnprintfce
#undef   _snprintf
#define  _snprintf   _snprintfce
#undef    vsprintf
#define   vsprintf    vsprintfce
#undef    strlen
#define   strlen      strlence
#ifndef _tcsnicmp
#define _tcsnicmp _strnicmp
#endif

char * __cdecl _strdup (const char * string);
size_t __cdecl strspn (const char * string, const char * control);
int __cdecl _stricmp(const char * dst, const char * src);
int __cdecl _strnicmp (const char * first, const char * last, size_t count);
char * __cdecl strrchr (const char * string, int ch);
char * __cdecl strpbrk (const char * string, const char * control);
char * __cdecl strcat (char * dst, const char * src);
char * __cdecl strcpy(char * dst, const char * src);
int __cdecl sprintf(char *, const char *, ...);
int __cdecl _vsnprintf(char *, size_t, const char *, va_list);
int __cdecl _snprintf(char *, size_t, const char *, ...);
int __cdecl vsprintf(char *Buffer, const char *Format, va_list arg_marker);
size_t __cdecl strlen (const char * str);


#undef  isdigit
#define isdigit(c) ( (c>='0') && (c<='9') )
#undef  isspace
#define isspace(c) (c==' ' || c=='\t' || c=='\n')

#ifndef _WIN32_WCE_EMULATION
#define printf    printfce
int __cdecl printf(const char *, ...);
#endif

#undef _tcsicmp
#define _tcsicmp _stricmp
#undef _tcsrchr
#define _tcsrchr strrchr

/* Functions that didn't exist in the CEx86 libraries but were in the Emulator
 * libraries.  Rather than defining them only for CEx86, we'll remap them here
 * so that both the WINCE and CEx86 builds will use the new versions we wrote,
 * so we can debug the new versions in the emulator (which is easier to debug
 * with).
 */
#define CharUpperA       CharUpperACE
#define CharLowerA       CharLowerACE
#define PeekMessageA     PeekMessageACE
#define PostMessageA     PostMessageACE
#define DefWindowProcA   DefWindowProcACE
#define DispatchMessageA DispatchMessageACE
#define GetMessageA      GetMessageACE
#define IsDialogMessageA IsDialogMessageACE
#define SendMessageA     SendMessageACE
#define GetWindowLongA   GetWindowLongACE
#define SetWindowLongA   SetWindowLongACE
#define GetObjectA       GetObjectACE

WINBASEAPI
HANDLE
WINAPI
CreateEventA(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCSTR lpName
    );

LPSTR WINAPI CharUpperA(LPSTR lpsz);
LPSTR WINAPI CharLowerA(LPSTR lpsz);

WINUSERAPI
BOOL
WINAPI
PeekMessageA(
    PMSG pMsg,
    HWND hWnd ,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax,
    UINT wRemoveMsg);

WINUSERAPI
BOOL
WINAPI
PostMessageA(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);

LRESULT
WINAPI
DefWindowProcA(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);

WINUSERAPI
LONG
WINAPI
DispatchMessageA(
    CONST MSG *lpMsg);

WINUSERAPI
BOOL
WINAPI
GetMessageA(
    LPMSG lpMsg,
    HWND hWnd,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax);

WINUSERAPI
BOOL
WINAPI
IsDialogMessageA(
    HWND hDlg,
    LPMSG lpMsg);

WINUSERAPI
LRESULT
WINAPI
SendMessageA(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);

LONG
WINAPI
GetWindowLongA(
    HWND hWnd,
    int nIndex);

LONG
WINAPI
SetWindowLongA(
    HWND hWnd,
    int nIndex,
    LONG dwNewLong);

int
WINAPI
GetObjectA(
    HGDIOBJ hgdiobj,
    int cbBuffer,
    LPVOID lpvObject
    );

/* BUGBUGCE ADU
 * Remap several socket calls
 * because WinCE sock does not
 * support polling.
 */
#ifdef recv
#undef recv
#undef recvfrom
#endif
#define recv recvCE
#define recvfrom recvfromCE

int __stdcall recv(SOCKET s, char * buf, int len, int flags);

int __stdcall recvfrom(SOCKET s, char FAR* buf, int len, int flags,
      struct sockaddr FAR* from, int FAR* fromlen);

void setBlockingCE (BOOL blocking);

/*
 * Redefine PeekMessage() because
 * the WinCE version of
 * PeekMessage() does not see
 * WM_PAINTS.
 */
#undef PeekMessage
#define PeekMessage PeekMessageCE
BOOL PeekMessage(LPMSG pMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);

/* Function for returning the size of the CE taskbar */
INT CETaskBarSize(void);

/* Function that returns keyboard scancodes. */
UINT CEGetScanCode(UINT);

/* Gets either hardware ID or retrieves stored random number id */
ULONG CEGetUniqueId();

#ifdef __cplusplus
}
#endif

#endif /* __CITRIX_CE__ */

/*This is the WCECALLS.CPP section for WCECALLS.LIB*/

#if defined(__cplusplus)
extern "C"
{
#endif
/* Undefine the standard windows call here or the emulation defines
 * in WCEEMUL.H will replace it with an emulation call if emulation is
 * on.  Then redef it as our xxxAW call.
 */

/* APIs to redef */
#undef GetVersionEx
#define GetVersionEx(x) GetVersionExW((LPOSVERSIONINFOW)(x))
#undef  LoadLibrary
#define LoadLibrary LoadLibraryAW
#undef  CreateFile
#define CreateFile CreateFileAW
#undef  CreateFileMapping
#define CreateFileMapping CreateFileMappingAW
#undef  MessageBox
#define MessageBox MessageBoxAW
#undef  CreateWindow
#define CreateWindow CreateWindowAW
#undef  RegisterClass
#define RegisterClass RegisterClassAW
#undef  UnregisterClass
#define UnregisterClass UnregisterClassAW
#undef  GetProcAddress
#define GetProcAddress GetProcAddressAW
#undef  GetFileAttributes
#define GetFileAttributes GetFileAttributesAW
#undef  SetFileAttributes
#define SetFileAttributes SetFileAttributesAW
#undef  FindFirstFile
#define FindFirstFile FindFirstFileAW
#undef  FindNextFile
#define FindNextFile FindNextFileAW
#undef  RegCreateKeyEx
#define RegCreateKeyEx RegCreateKeyExAW
#undef  RegOpenKeyEx
#define RegOpenKeyEx RegOpenKeyExAW
#undef  RegQueryValueEx
#define RegQueryValueEx RegQueryValueExAW
#undef  RegSetValueEx
#define RegSetValueEx RegSetValueExAW
#undef  RegDeleteKey
#define RegDeleteKey RegDeleteKeyAW
#undef  RegDeleteValue
#define RegDeleteValue RegDeleteValueAW
#undef  RegQueryInfoKey
#define RegQueryInfoKey RegQueryInfoKeyAW
#undef  RegEnumKeyEx
#define RegEnumKeyEx RegEnumKeyExAW
#undef  RegEnumValue
#define RegEnumValue RegEnumValueAW
#undef  CreateDialog
#define CreateDialog CreateDialogAW
#undef  LoadString
#define LoadString LoadStringAW
#undef  GetModuleFileName
#define GetModuleFileName GetModuleFileNameAW
#undef  ExtTextOut
#define ExtTextOut ExtTextOutAW
#undef  FindWindow
#define FindWindow FindWindowAW
#undef  GetTextMetrics
#define GetTextMetrics GetTextMetricsAW
#undef  WinExec
#define WinExec WinExecAW
#undef  GetWindowText
#define GetWindowText GetWindowTextAW
#undef  InsertMenu
#define InsertMenu InsertMenuAW
#undef  LoadIcon
#define LoadIcon LoadIconAW
#undef  SetDlgItemText
#define SetDlgItemText SetDlgItemTextAW
#undef  SetWindowText
#define SetWindowText SetWindowTextAW
#undef  CreateMutex
#define CreateMutex CreateMutexAW
#undef  CreateDC
#define CreateDC CreateDCAW
#undef  DeleteFile
#define DeleteFile DeleteFileAW
#undef  CreateDirectory
#define CreateDirectory CreateDirectoryAW
#undef  RemoveDirectory
#define RemoveDirectory RemoveDirectoryAW
#undef  MoveFile
#define MoveFile MoveFileAW
#undef  CreateEvent
#define CreateEvent CreateEventAW
#undef  AppendMenu
#define AppendMenu AppendMenuAW
#undef  RegisterWindowMessage
#define RegisterWindowMessage RegisterWindowMessageAW
#undef  DrawText
#define DrawText DrawTextAW
#undef  CallWindowProc
#define CallWindowProc CallWindowProcW

#undef  RegisterClipboardFormat
#define RegisterClipboardFormat RegisterClipboardFormatAW
#undef  GetClipboardFormatName
#define GetClipboardFormatName GetClipboardFormatNameAW

#undef FindResource 
#define FindResource FindResourceAW
#undef CreateProcess
#define CreateProcess CreateProcessAW
#undef LoadMenu
#define LoadMenu LoadMenuAW

#undef OutputDebugString
#define OutputDebugString OutputDebugStringAW

#undef DialogBoxIndirectParamA
#define DialogBoxIndirectParamA DialogBoxIndirectParamW

/*Structures to redef*/
#undef _WIN32_FIND_DATA
#undef WIN32_FIND_DATA
#undef PWIN32_FIND_DATA
#undef LPWIN32_FIND_DATA
#define _WIN32_FIND_DATA  _WIN32_FIND_DATAA
#define WIN32_FIND_DATA   WIN32_FIND_DATAA
#define PWIN32_FIND_DATA  PWIN32_FIND_DATAA
#define LPWIN32_FIND_DATA LPWIN32_FIND_DATAA

/* Defs needed for file functions */
#define OF_READ             0x00000000
#define OF_WRITE            0x00000001
#define OF_READWRITE        0x00000002
#define OF_SHARE_COMPAT     0x00000000
#define OF_SHARE_EXCLUSIVE  0x00000010
#define OF_SHARE_DENY_WRITE 0x00000020
#define OF_SHARE_DENY_READ  0x00000030
#define OF_SHARE_DENY_NONE  0x00000040

#ifndef _S_IREAD
#define _S_IREAD  OF_READ
#define _S_IWRITE OF_WRITE
#endif

/* The following are function calls that force xxxxA APIs to call xxxxW APIs */

HINSTANCE
WINAPI
LoadLibraryAW(
    LPCSTR lpLibFileName
    );

HANDLE
WINAPI
CreateFileAW(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    );

HANDLE
WINAPI
CreateFileMappingAW(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCSTR lpName
    );

int
WINAPI
MessageBoxAW(
    HWND hWnd ,
    LPCSTR lpText,
    LPCSTR lpCaption,
    UINT uType);

HWND
WINAPI
CreateWindowAW(
    LPCSTR lpClassName,
    LPCSTR lpWindowName,
    DWORD dwStyle,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam);

ATOM
WINAPI
RegisterClassAW(
    CONST WNDCLASSA *lpWndClass
    );

BOOL
WINAPI
UnregisterClassAW(
    LPCSTR lpClassName,
    HINSTANCE hInstance);

FARPROC
WINAPI
GetProcAddressAW(
    HMODULE hModule,
    LPCSTR lpProcName
    );

DWORD
WINAPI
GetFileAttributesAW(
    LPCSTR lpFileName
    );

BOOL
WINAPI
SetFileAttributesAW (
    LPCSTR lpszName,
    DWORD dwAttributes
    );

HANDLE
WINAPI
FindFirstFileAW(
    LPCSTR lpFileName,
    LPWIN32_FIND_DATAA lpFindFileData
    );

BOOL
WINAPI
FindNextFileAW(
    HANDLE hFindFile,
    LPWIN32_FIND_DATAA lpFindFileData
    );

LONG
APIENTRY
RegCreateKeyExAW (
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD Reserved,
    LPSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    );

LONG
APIENTRY
RegOpenKeyExAW (
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    );

LONG
APIENTRY
RegQueryValueExAW (
    HKEY hKey,
    LPCSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

LONG
APIENTRY
RegSetValueExAW (
    HKEY hKey,
    LPCSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    CONST BYTE* lpData,
    DWORD cbData
    );

LONG
APIENTRY
RegDeleteKeyAW (
    HKEY hKey,
    LPCSTR lpSubKey
    );

LONG
APIENTRY
RegDeleteValueAW (
    HKEY hKey,
    LPCSTR lpValueName
    );

LONG
APIENTRY
RegQueryInfoKeyAW (
    HKEY hKey,
    LPSTR lpClass,
    LPDWORD lpcbClass,
    LPDWORD lpReserved,
    LPDWORD lpcSubKeys,
    LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcbMaxClassLen,
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueNameLen,
    LPDWORD lpcbMaxValueLen,
    LPDWORD lpcbSecurityDescriptor,
    PFILETIME lpftLastWriteTime
    );

LONG
APIENTRY
RegEnumKeyExAW (
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpName,
    LPDWORD lpcbName,
    LPDWORD lpReserved,
    LPSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime
    );


LONG
APIENTRY
RegEnumValueAW (
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpszValueName,
    LPDWORD lpchValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );



UINT
WINAPI
RegisterWindowMessageAW (
    LPCSTR
    );


WINUSERAPI
int
WINAPI
DrawTextAW (
    HDC,
    LPCSTR,
    int,
    RECT *,
    UINT);

VOID
WINAPI
OutputDebugStringAW(
    LPCSTR lpOutputString
    );

/* Flushing in WinCE is automatic since
 * you are writting directly to memory.
 */
#define RegFlushKey(a) ERROR_SUCCESS


WINGDIAPI BOOL  WINAPI UpdateColors(HDC);

int __cdecl _open(const char * name, int oflag, ...);
int __cdecl rename(const char * oldname, const char * newname);

HFILE
WINAPI
_lopen(
    LPCSTR lpPathName,
    int iReadWrite
    );

HFILE
WINAPI
_lcreat(
    LPCSTR lpPathName,
    int  iAttribute
    );

UINT
WINAPI
_lread(
    HFILE hFile,
    LPVOID lpBuffer,
    UINT uBytes
    );

UINT
WINAPI
_lwrite(
    HFILE hFile,
    LPCSTR lpBuffer,
    UINT uBytes
    );

HFILE
WINAPI
_lclose(
    HFILE hFile
    );

LONG
WINAPI
_llseek(
    HFILE hFile,
    LONG lOffset,
    int iOrigin
    );

HWND
WINAPI
CreateDialogAW(
    HINSTANCE hInstance,
    LPCTSTR lpTemplate,
    HWND hWndParent,
    DLGPROC lpDialogFunc
    );

int
WINAPI
LoadStringAW(
    HINSTANCE hInstance,
    UINT uID,
    LPSTR lpBuffer,
    int nBufferMax);

DWORD
WINAPI
GetModuleFileNameAW(
    HMODULE hMod,
    LPSTR lpBuffer,
    DWORD dwSize);

WINGDIAPI
BOOL
WINAPI
ExtTextOutAW(
    HDC hdc,
    int x,
    int y,
    UINT fuOptions,
    CONST RECT *lprc,
    LPCSTR lpString,
    UINT cbCount,
    CONST INT *lpDx);

WINUSERAPI
HWND
WINAPI
FindWindowAW(
    LPCSTR lpClassName ,
    LPCSTR lpWindowName);

WINGDIAPI
BOOL
WINAPI
GetTextMetricsAW(
    HDC hdc,
    LPTEXTMETRICA lptm);

int
WINAPI
GetWindowTextAW(
    HWND hWnd,
    LPSTR lpString,
    int nMaxCount);

BOOL
WINAPI
InsertMenuAW(
    HMENU hMenu,
    UINT uPosition,
    UINT uFlags,
    UINT uIDNewItem,
    LPCSTR lpNewItem);

HICON
WINAPI
LoadIconAW(
    HINSTANCE hInstance,
    LPCSTR lpIconName);

WINUSERAPI
BOOL
WINAPI
SetDlgItemTextAW(
    HWND hDlg,
    int nIDDlgItem,
    LPCSTR lpString);

BOOL
WINAPI
SetWindowTextAW(
    HWND hWnd,
    LPCSTR lpString);


/* Define in terms of ExtractIconEx */
HICON ExtractIcon(HINSTANCE hInst, LPCSTR lpszExeFileName, UINT nIconIndex);

/* WinExec() not supported.
 * Map to CreateProcess().
 */
   UINT WinExec(LPCSTR lpCmdLine, UINT uCmdShow);

HANDLE
WINAPI
CreateMutex(
   LPSECURITY_ATTRIBUTES lpsa,
   BOOL bInitialOwner,
   LPCSTR lpName);

HDC
WINAPI
CreateDC(
   LPCTSTR lpszDriver,
   LPCTSTR lpszDevice,
   LPCTSTR lpszOutput,
   CONST DEVMODE *lpInitData);

BOOL
WINAPI
DeleteFileAW(
    LPCSTR lpFileName
    );

BOOL
WINAPI
CreateDirectoryAW(
    LPCSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

BOOL
WINAPI
RemoveDirectoryAW(
    LPCSTR lpPathName
    );

BOOL
WINAPI
MoveFileAW(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName
    );

HANDLE
WINAPI
CreateEventAW(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPTSTR lpname);

UINT
WINAPI
RegisterClipboardFormatAW(
    LPCSTR lpszFormat
    );

int
WINAPI
GetClipboardFormatNameAW(
    UINT format,
    LPSTR lpszFormatName,
    int cchMaxCount
    );

BOOL
WINAPI
AppendMenuAW(
        HMENU,
        UINT,
        UINT,
        LPCSTR
        );

HMENU
WINAPI
LoadMenuAW(
    HINSTANCE hInstance,
    LPCSTR lpMenuName);

HRSRC
WINAPI
FindResourceAW(
    HMODULE hModule,
    LPCSTR lpName,
    LPCSTR lpType
    );

BOOL
WINAPI
CreateProcessAW(
    LPCSTR pszImageName,
    LPCSTR pszCmdLine,
    LPSECURITY_ATTRIBUTES psaProcess,
    LPSECURITY_ATTRIBUTES psaThread,
    BOOL fInheritHandles,
    DWORD fdwCreate,
    LPVOID pvEnvironment,
    LPSTR pszCurDir,
    LPSTARTUPINFO psiStartInfo,
    LPPROCESS_INFORMATION pProcInfo
    );

ULONG CEGetKeyboardLayout(void);

int CEGetKeyboardType(int nTypeFlag);

UINT CEGetVirtualKey(UINT ScanCode);

BOOL    RunningOnWBT(void);

BOOL    RunningOnPalmPC(void);

BOOL    RunningOnLPalm(void);

BOOL    RunningOnWyseWinterm(void);

BOOL    ClientUpdateAllowed(void);

BOOL    PreferredKeyboardModeIsUnicode(void);

BOOL    PreferToUseLocalIME(void);

CHAR *  GetCtxMoveProgramName(void);

UINT    SimpleMapVirtualKeyW( UINT uCode, UINT uMapType);

void    CreateFullPathName(char * fullname, const char * name);

void    CEBeep(DWORD dwFreq, DWORD dwDuration);

int     GetSessionDepth(void);

void    SetSessionDepth(int depth);

HWND    GetDesktopWindow(void);

LONG    RegFlushKeyCE( HKEY hKey );

#if defined(ZeroMemory)
#undef ZeroMemory
#endif

VOID ZeroMemory(void *Destination, long Length);

VOID CopyMemory(void * Destination, void * Source, long Length);

HWND GetDesktopWindow(void);

#ifndef _read
int __cdecl _mkdir(const char * name);
int __cdecl _rmdir(const char * name);
int __cdecl _remove(const char * name);
int __cdecl _open(const char * name, int oflag, ...);
int __cdecl _read( int handle, void *buffer, unsigned int size );
int __cdecl _write( int handle, const void *buffer, unsigned int size );
long __cdecl _lseek(int fd, long offset, int whence);
int __cdecl _close( int fd );
long __cdecl _filelength(int fd);
int __cdecl _chsize(int fd, long size);
int __cdecl _access(const char *path, int mode);
#endif
#endif  /* #ifdef WINCE */
#if defined(__cplusplus)
}
#endif
#endif /* !UNICODE */
#endif /* _WCECALLS_H_ */
#endif /* defined WINCE */
