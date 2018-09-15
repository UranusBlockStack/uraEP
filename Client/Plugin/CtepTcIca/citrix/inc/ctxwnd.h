/*******************************************************************************
 *
 *  ctxwnd.h
 *
 *  Window Object Interface
 *
 *  Copyright (c) 1999 - 2003 Citrix Systems, Inc. All Rights Reserved.
 *
 ******************************************************************************/

#ifndef __CTXWND_H_
#define __CTXWND_H_

#include <ctxgraph.h> /* reference to HGCURSOR */
#include <wdapi.h>
#ifdef __cplusplus
extern "C" {
#endif

#ifndef OUT
#define OUT
#endif

#ifndef IN
#define IN
#endif

#define WND_STATUS_SUCCESS             0
#define WND_STATUS_BADHANDLE           1 /* abstract window handle is wrong */
#define WND_STATUS_BADPARAM            2 /* parameters to a function is bad */
#define WND_STATUS_NOMEMORY            3 /* failure to allocate memory */
#define WND_STATUS_INSUFFICIENT_BUFFER 4 /* insufficient buffer size */
#define WND_STATUS_ERROR               5 /* miscellaneous error message */


/*  Flags used by the Wnd_get/setKeyboardState function.
 */
#define WND_KBDSTATE_CAPSLOCK   (0x00000001)
#define WND_KBDSTATE_NUMLOCK    (0x00000002)
#define WND_KBDSTATE_SCROLLLOCK (0x00000004)


/***************************************************************
 * Window event structure delivered using pfnDeliver function.
 *
 ***************************************************************/
typedef struct WND_EVENTINFO
{
    HND   hSource;                    /* the source window */
    UINT8 uiEventID;                  /* indicates the type of window event */
#define WND_EVTID_MOUSEINFO    1       /* mouse data delivered (WND_MOUSEINFO) */
#define WND_EVTID_SCANCODEINFO 2       /* keyboard scancodes delivered (WND_SCANCODEINFO) */
#define WND_EVTID_CHARCODEINFO 3       /* keyboard charcodes delivered (WND_CHARCODEINFO) */
#define WND_EVTID_UNICODEINFO  4       /* keyboard unicode delivered (WND_UNICODEINFO) */
#define WND_EVTID_FOCUSINFO    5       /* window focus has changed (WND_FOCUSINFO) */
#define WND_EVTID_PAINTINFO    6       /* window must be repainted (WND_PAINTINFO) */
#define WND_EVTID_BOUNDSINFO   7      /* window has been moved/resized (WND_BOUNDSINFO) */
#define WND_EVTID_UNICODEINSERT 8      /* Direct UNICODE char insertion (WND_UNICODECHARINFO) */
    /* etc... */
    
}
WND_EVENTINFO, TYPEDEF_FAR * PWND_EVENTINFO;



typedef enum _WND_TYPE
{
    WND_TYPE_EMBEDDED,   /* a child window to be placed within a parent window  */
    WND_TYPE_OTHER       /* an unknown type (used for natively created windows) */
}
WND_TYPE;


typedef struct WND_MOUSEDATA
{
    UINT16 X;
    UINT16 Y;
    UINT8  cMouState;
#define WND_MOUSEDATA_MOVED  0x01
#define WND_MOUSEDATA_B1DOWN 0x02
#define WND_MOUSEDATA_B1UP   0x04
#define WND_MOUSEDATA_B2DOWN 0x08
#define WND_MOUSEDATA_B2UP   0x10
#define WND_MOUSEDATA_B3DOWN 0x20
#define WND_MOUSEDATA_B3UP   0x40
#define WND_MOUSEDATA_DBLCLK 0x80
#define WND_MOUSEDATA_B4DOWN 0x02
#define WND_MOUSEDATA_B4UP   0x04
#define WND_MOUSEDATA_B5DOWN 0x08
#define WND_MOUSEDATA_B5UP   0x10
#define WND_MOUSEDATA_WHEEL  0x20
    
    UINT8  uiType;
#define WND_MOUSEDATA_NORMAL   0
#define WND_MOUSEDATA_EXTRA    1
    UINT16 uiExtraInfo;
}
WND_MOUSEDATA, TYPEDEF_FAR * PWND_MOUSEDATA;

typedef struct WND_MOUSEINFO
{
    WND_EVENTINFO info;
    UINT16         cMouseData;         /* the number of WND_MOUSEDATA structures */
    PWND_MOUSEDATA pMouseData;         /* an array of WND_MOUSEDATA structures */
}
WND_MOUSEINFO, TYPEDEF_FAR * PWND_MOUSEINFO;



typedef struct WND_SCANCODEINFO
{    
    WND_EVENTINFO info;
    UINT16 cScanCodes;
    PUCHAR  pScanCodes;
    UINT16 uiRepeat;
    UINT16 uiShiftState;
}
WND_SCANCODEINFO, TYPEDEF_FAR * PWND_SCANCODEINFO;


typedef struct WND_CHARCODEINFO
{
    WND_EVENTINFO info;
    UINT16 cCharCodes;
    LPBYTE pCharCodes;
    UINT16 uiRepeat;
    UINT16 uiShiftState;
}
WND_CHARCODEINFO, TYPEDEF_FAR * PWND_CHARCODEINFO;

typedef struct WND_UNICODECHAR
{
    UINT8  uiFlags;               /* the type of char value */
#define WND_UNICODECHAR_KEYUP   0x01
#define WND_UNICODECHAR_SPECIAL 0x02
#define WND_UNICODECHAR_DIRECT  0x04
    UINT16 uiValue;               /* Unicode or special key value */
}
WND_UNICODECHAR, TYPEDEF_FAR * PWND_UNICODECHAR;


typedef struct WND_UNICODEINFO
{
    WND_EVENTINFO   info;
    UINT16          cUnicodeCodes;
    WND_UNICODECHAR UnicodeCodes;         
    UINT16          uiRepeat;
    UINT16          uiShiftState;
}
WND_UNICODEINFO, * PWND_UNICODEINFO;


/* Direct unicode character insertion */
typedef struct WND_UNICODECHARINFO
{
    WND_EVENTINFO   info;     
    WND_UNICODECHAR uc;          /* Unicode character */
    UINT16          uiRepeat;
}
WND_UNICODECHARINFO, * PWND_UNICODECHARINFO;


/*  This event is delivered when the focus state of a window is changing. */
typedef struct WND_FOCUSINFO
{
    WND_EVENTINFO info;
    UINT32        uiState;
#define WND_FOCUSINFO_GAINING 1
#define WND_FOCUSINFO_GAINED  2
#define WND_FOCUSINFO_LOSING  3
#define WND_FOCUSINFO_LOST    4
    /* ...etc... */
}
WND_FOCUSINFO, * PWND_FOCUSINFO;


typedef struct WND_PAINTINFO
{
    WND_EVENTINFO info;
}
WND_PAINTINFO, * PWND_PAINTINFO;


/* structure to store various input preferences */
typedef struct WND_INPUTPREFERENCES
{
    UINT16  uiKeyboardDelay;
    UINT16  uiKeyboardSpeed;
    BOOL    fSwapButtons;
    UINT16  uiHorizSpeed;
    UINT16  uiVertSpeed;
    UINT16  uiDblSpeedThreshold;
}
WND_INPUTPREFERENCES, *PWND_INPUTPREFERENCES;

#define IMEFILENAME_LENGTH    32
typedef struct WND_KEYBOARDINFO
{
    UINT32 uiKeyboardLayout;
    UINT16 uiKeyboardType;          /* Keyboard type (PC/XT, PC/AT, Japanese...) */
    UINT8  uiKeyboardSubType;       /* Keyboard subtype (US 101, JPN 106) */
    UINT8  uiKeyboardFunctionKey;   /* Number of function keys */
    UINT16 uiKeyboardSubtype2;
    CHAR   imeFileName[IMEFILENAME_LENGTH+1];          /* IME file name */
    /* Flag to decide if Client can send left and right windows keys through */
    BOOL   fWinKeyFlag;		
}
WND_KEYBOARDINFO, *PWND_KEYBOARDINFO;


typedef VPSTATUS (_PVPAPI PFNWNDCREATE)(WND_TYPE, PVPRECT, HND, PVOID, PFNDELIVER,HND,PHND);
typedef VPSTATUS (_PVPAPI PFNWNDSETUSERDATA)(HND,PVOID);
typedef VPSTATUS (_PVPAPI PFNWNDGETUSERDATA)(HND,PPVOID);
typedef VPSTATUS (_PVPAPI PFNWNDSETPARENT)(HND, HND);
typedef VPSTATUS (_PVPAPI PFNWNDSETVISIBLE)(HND, BOOL);
typedef VPSTATUS (_PVPAPI PFNWNDSETSIZE)(HND, PVPSIZE);
typedef VPSTATUS (_PVPAPI PFNWNDGETSIZE)(HND, PVPSIZE);
typedef VPSTATUS (_PVPAPI PFNWNDSETPOS)(HND, PVPPOINT);
typedef VPSTATUS (_PVPAPI PFNWNDREPAINTRECT)(HND, PVPRECT);
typedef VPSTATUS (_PVPAPI PFNWNDGETREPAINTRECTS)(HND,PVPRECT,INT16,PINT16);
typedef VPSTATUS (_PVPAPI PFNWNDGETOBSCUREDRECTS)(HND,PVPRECT,PVPRECT,INT16,PINT16);
typedef VPSTATUS (_PVPAPI PFNWNDSETINPUTPREFERENCES)(PWND_INPUTPREFERENCES);
typedef VPSTATUS (_PVPAPI PFNWNDGETKEYBOARDINFO)(UINT32,UINT32,CHAR*,PWND_KEYBOARDINFO);
typedef VPSTATUS (_PVPAPI PFNWNDGETKEYBOARDSTATE)(PUINT32);
typedef VPSTATUS (_PVPAPI PFNWNDSETKEYBOARDSTATE)(UINT32);
typedef VPSTATUS (_PVPAPI PFNWNDSETCURSOR)( HND, HGCURSOR );
typedef VPSTATUS (_PVPAPI PFNWNDSETMOUSEPOS)( INT32, INT32, HND );
typedef VPSTATUS (_PVPAPI PFNWNDREQUESTFOCUS)( HND );
typedef VPSTATUS (_PVPAPI PFNWNDDESTROY)(PHND);


typedef struct _WNDCALLTABLE
{
    PFNWNDCREATE              pfnWndCreate;
    PFNWNDSETUSERDATA         pfnWndSetUserData;
    PFNWNDGETUSERDATA         pfnWndGetUserData;
    PFNWNDSETPARENT           pfnWndSetParent;
    PFNWNDSETVISIBLE          pfnWndSetVisible;
    PFNWNDSETSIZE             pfnWndSetSize;
    PFNWNDGETSIZE             pfnWndGetSize;
    PFNWNDSETPOS              pfnWndSetPos;
    PFNWNDREPAINTRECT         pfnWndRepaintRect;
    PFNWNDGETREPAINTRECTS     pfnGetRepaintRects;
    PFNWNDGETOBSCUREDRECTS    pfnGetObscuredRects;
    PFNWNDSETINPUTPREFERENCES pfnWndSetInputPreferences;
    PFNWNDGETKEYBOARDINFO     pfnWndGetKeyboardInfo;
    PFNWNDGETKEYBOARDSTATE    pfnWndGetKeyboardState;
    PFNWNDSETKEYBOARDSTATE    pfnWndSetKeyboardState;
    PFNWNDSETCURSOR           pfnWndSetCursor;
    PFNWNDSETMOUSEPOS         pfnWndSetMousePos;
    PFNWNDREQUESTFOCUS        pfnWndRequestFocus;
    PFNWNDDESTROY             pfnWndDestroy;
} WNDCALLTABLE, FAR * PWNDCALLTABLE, FAR * FAR * PPWNDCALLTABLE;

#define WND_STRING        "CTXWND"

VPSTATUS _VPAPI Wnd_Load(PPWNDCALLTABLE ppWndCallTable);
VPSTATUS _VPAPI Wnd_Unload();

#ifdef  USE_WND_CALL_TABLE

#define WND_GLOBAL_INIT   PWNDCALLTABLE g_pWndCallTable = NULL
#define WND_EXTERN_INIT   extern PWNDCALLTABLE g_pWndCallTable 

WND_EXTERN_INIT;

#define Wnd_create               (*g_pWndCallTable->pfnWndCreate)
#define Wnd_setUserData          (*g_pWndCallTable->pfnWndSetUserData)
#define Wnd_getUserData          (*g_pWndCallTable->pfnWndGetUserData)
#define Wnd_setParent            (*g_pWndCallTable->pfnWndSetParent)
#define Wnd_setVisible           (*g_pWndCallTable->pfnWndSetVisible)
#define Wnd_setSize              (*g_pWndCallTable->pfnWndSetSize)
#define Wnd_getSize              (*g_pWndCallTable->pfnWndGetSize)
#define Wnd_setPos               (*g_pWndCallTable->pfnWndSetPos)
#define Wnd_repaintRect          (*g_pWndCallTable->pfnWndRepaintRect)
#define Wnd_getRepaintRects      (*g_pWndCallTable->pfnGetRepaintRects)
#define Wnd_getObscuredRects     (*g_pWndCallTable->pfnGetObscuredRects)
#define Wnd_setInputPreferences  (*g_pWndCallTable->pfnWndSetInputPreferences)
#define Wnd_getKeyboardInfo      (*g_pWndCallTable->pfnWndGetKeyboardInfo)
#define Wnd_getKeyboardState     (*g_pWndCallTable->pfnWndGetKeyboardState)
#define Wnd_setKeyboardState     (*g_pWndCallTable->pfnWndSetKeyboardState)
#define Wnd_setCursor            (*g_pWndCallTable->pfnWndSetCursor)
#define Wnd_setMousePos          (*g_pWndCallTable->pfnWndSetMousePos)
#define Wnd_requestFocus         (*g_pWndCallTable->pfnWndRequestFocus)
#define Wnd_destroy              (*g_pWndCallTable->pfnWndDestroy)

#define WndRunTimeInit( libmgr, pver ) \
    ((*(libmgr->pLMgrUse))( WND_STRING, pver, (PPLMGRTABLE)&g_pWndCallTable ))
#define WndRuntimeRelease( libmgr ) \
    ((*(libmgr->pLMgrRelease))( (PLMGRTABLE)g_pWndCallTable ))

#else /* !USE_WND_CALL_TABLE */

#define WND_GLOBAL_INIT  
#define WND_EXTERN_INIT   

#define WndRunTimeInit( libmgr, pver )
#define WndRuntimeRelease( libmgr ) 

/*******************************************************************************
 *
 *  Exported Functions
 *
 *    Wnd_create
 *    Wnd_setUserData
 *    Wnd_getUserData
 *    Wnd_setParent
 *    Wnd_setVisible
 *    Wnd_setSize
 *    Wnd_getSize
 *    Wnd_setPos
 *    Wnd_repaintRect 
 *    Wnd_getRepaintRects
 *    Wnd_getObscuredRects
 *    Wnd_setCursor
 *    Wnd_setMousePos
 *    Wnd_requestFocus
 *    Wnd_destroy
 *
 ******************************************************************************/



/* windows APIs defined in wndapi.c */

VPSTATUS _VPAPI
Wnd_create(IN  WND_TYPE     type,         /* the type of window to create */
           IN  PVPRECT      pBounds,      /* the size and position of the new window */
           IN  HND          hParent,      /* opt. parent window handle */
           IN  PVOID        pSubscriber,  /* pointer to window event subscriber */
           IN  PFNDELIVER   pfnDeliver, /* window event delivery function */
           IN  HND          hThread,      /* thread to deliver events */
           OUT PHND         phWindow );

VPSTATUS _VPAPI
Wnd_setUserData(IN HND   hWindow,       /* handle of window */
                IN PVOID pUserData ) ;  /* pointer to user-defined structure */

VPSTATUS _VPAPI
Wnd_getUserData(IN  HND    hWindow,       /* handle of window */
                OUT PPVOID ppUserData );  /* output var. for pointer to user-defined structure */
VPSTATUS _VPAPI
Wnd_setParent(IN HND hWindow,   /* handle of window */
              IN HND hParent ); /* handle of new parent window */


VPSTATUS _VPAPI
Wnd_setVisible(IN HND  hWindow,          /* handle of window */
               IN BOOL fVisible ) ;   /* new visibility state */

VPSTATUS _VPAPI
Wnd_setSize(IN HND     hWindow, /* handle of window */
            OUT PVPSIZE pSize );/* new window size (width and height) */

VPSTATUS _VPAPI
Wnd_getSize(IN HND     hWindow, /* handle of window */
            IN PVPSIZE pSize ); /*  window size (width and height) */

VPSTATUS _VPAPI
Wnd_setPos(IN HND      hWindow,          /* handle of window */
           IN PVPPOINT pPos ) ;          /* new window position (x and y) */

VPSTATUS _VPAPI
Wnd_repaintRect(IN HND     hWindow,   /* handle of window */
                IN PVPRECT pRegion);   /* new window bounds */

VPSTATUS _VPAPI
Wnd_getRepaintRects( IN  HND      hWindow,    /* handle of window */
                     OUT PVPRECT  pRects,     /* a buffer for storing rects */
                     IN  INT16    cRects,     /* max number of rects to store in buffer */
                     OUT PINT16   pcRects ) ; /* rectangles stored */
                
VPSTATUS _VPAPI
Wnd_getObscuredRects( IN  HND     hWindow,    /* handle of window */
                      IN  PVPRECT pBounds,    /* optional bounding rect */
                      OUT PVPRECT pRects,     /* a buffer for storing rects */
                      IN  INT16   cRects,     /* max number of rects to store in buffer */
                      OUT PINT16  pcRects ) ; /* rectangles stored */

VPSTATUS _VPAPI
Wnd_setInputPreferences (IN PWND_INPUTPREFERENCES pPreferences); /* pointer to preferences */

VPSTATUS _VPAPI
Wnd_getKeyboardInfo(IN  UINT32 uiKeyboard, /* Kbd type code*/
                    IN  UINT32 uiLayout, /* Keyboard layout*/
                    IN  CHAR*  szKeyboardLayout, /* Layout */
                    OUT PWND_KEYBOARDINFO pKeybordInfo);/* Pointer to structure with kbd info*/

VPSTATUS _VPAPI
Wnd_getKeyboardState( PUINT32 );

VPSTATUS _VPAPI
Wnd_setKeyboardState( UINT32 );

VPSTATUS _VPAPI
Wnd_setCursor( HND hWindow, HGCURSOR hCursor ) ;

VPSTATUS _VPAPI
Wnd_setMousePos( INT32 iX, INT32 iY, HND hWindow );

VPSTATUS _VPAPI
Wnd_requestFocus( HND hWindow );

VPSTATUS _VPAPI
Wnd_destroy(PHND phWindow ) ;    /* pointer to window handle */

#endif /* USE_WND_CALL_TABLE */

#ifdef __cplusplus
}
#endif


#endif /* __CTXWND_H_ */


