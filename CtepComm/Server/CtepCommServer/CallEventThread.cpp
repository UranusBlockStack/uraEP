#include "stdafx.h"

#include "AppChannel.h"

#include "WtsApi32.h"
#pragma comment(lib,"Wtsapi32.lib")


#define  MONITOR_WND_CLASS_BASE_NAME	"CTepCallBackEventClass"

static Log4CppLib g_log("CBkEvt");
static WCHAR WindowName[512];
static CCtepCommunicationServer* pThis;

//窗口消息处理函数
static LRESULT CALLBACK SessionMonitorWndProc( HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_WTSSESSION_CHANGE:
		ASSERT(pThis);
		if ( pThis)
			pThis->m_CallBackList_SessionEvent.evtSessionMessage(wParam, lParam);
		return 0;

	case WM_CLOSE:
		pThis = NULL;
		DestroyWindow(hWnd);
		return 0;

	case WM_QUIT:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}
static BOOL RegisterMonitorWndClass()
{
	WNDCLASSEX wc;
	swprintf_s(WindowName, L"%s-%d", MONITOR_WND_CLASS_BASE_NAME, GetCurrentProcessId());

	memset((LPVOID)&wc, 0, sizeof(WNDCLASSEX) );
	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = CS_VREDRAW | CS_HREDRAW ;
	wc.lpfnWndProc   = SessionMonitorWndProc;
	wc.hInstance     = GetModuleHandle(NULL);
	wc.lpszMenuName  = WindowName;
	wc.lpszClassName = WindowName;

	if( !RegisterClassEx(&wc))
	{
		return FALSE;
	}
	return TRUE;
}
static void UnRegisterMonitorWndClass()
{
	UnregisterClass(WindowName, GetModuleHandle(NULL));
	ZeroArray(WindowName);
}

HWND CreateMonitorWindows()
{
	HWND hWnd = CreateWindow(WindowName, WindowName, WS_OVERLAPPED, 
		0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandle( NULL ), NULL );
	UpdateWindow(hWnd);

	return hWnd;
}

DWORD CCtepCommunicationServer::TrdCallBackEventWorker(LPVOID param, HANDLE hEvtQuit)
{
	UNREFERENCED_PARAMETER(param);
	HWND h_MainWnd;
	DWORD dwResult = 0;

	pThis = this;
	if ( !RegisterMonitorWndClass())//注册一个用于监视的窗口类
	{
		dwResult = (DWORD)-2;
		g_log.ErrorW(5, L"TrdCallBackEventWorker - RegisterMonitorWndClass() failed.");
		goto End;
	}

	//创建一个主窗口
	h_MainWnd = CreateMonitorWindows();
	if( !h_MainWnd)
	{
		dwResult = (DWORD)-3;
		g_log.ErrorW(5, L"TrdCallBackEventWorker - CreateMonitorWindows() failed.");
		goto End;
	}

	//等待Terminal Server启动，然后用WTSRegisterSessionNotification注册会话通知消息
	//如果在RDS服务启动前调用WTSRegisterSessionNotification函数，会返回RPC_S_INVALID_BINDING错误
	HANDLE hTermSrvReadyEvent = NULL;
	DWORD dwCount = 0;
	while ( !hTermSrvReadyEvent && ++dwCount < 100)
	{
		hTermSrvReadyEvent = OpenEvent(SYNCHRONIZE, FALSE, _T("Global\\TermSrvReadyEvent"));
		if ( !hTermSrvReadyEvent)
			Sleep(500);
	}
	if( !hTermSrvReadyEvent)
	{
		g_log.ErrorW(5, L"Terminal Server is out of service.");
		goto End;
	}
	CloseHandle(hTermSrvReadyEvent);

	if( WTSRegisterSessionNotification(h_MainWnd, NOTIFY_FOR_ALL_SESSIONS) == FALSE )
	{
		g_log.ErrorW(5, L"4");
		goto End;
	}

	MSG msg;
	while(TRUE)
	{
		DWORD dwWait = MsgWaitForMultipleObjects(1, &hEvtQuit, FALSE, INFINITE, QS_ALLEVENTS);
		if ( dwWait == WAIT_OBJECT_0 + 1)
		{
			if ( PeekMessage(&msg, 0, 0, 0 ,PM_REMOVE))
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg);
			}
		}
		else if ( dwWait == WAIT_OBJECT_0)	// hEvtQuit事件受信
		{
			DestroyWindow(h_MainWnd);
			while ( IsWindow(h_MainWnd) && GetMessage(&msg, 0, 0, 0))
			{
				TranslateMessage( &msg);
				DispatchMessage( &msg);
			}

			break;
		}
	}

End:
	pThis = NULL;
	UnRegisterMonitorWndClass();

	return dwResult;
}