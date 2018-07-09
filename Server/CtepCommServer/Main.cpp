#include "stdafx.h"

#include "AppChannel.h"

#include <Locale.h>
#include "CommonInclude/ServiceImpl.h"
#include "CommonInclude/MainImpl.h"


BOOL WINAPI HandlerRoutine(__in  DWORD dwCtrlType);

class CMyService : public CCtepCommunicationServer
	, public CRunOneServiceImpl<CMyService>
	, public CServiceImpl<CMyService>
{
public:
	CMyService():CServiceImpl(L"CTEP", L"CTEP Foundation Architecture")
	{
#ifdef _DEBUG
		BOOL bRet = SetConsoleCtrlHandler(HandlerRoutine, TRUE);
		ASSERT(bRet);
#endif // _DEBUG
	}
	virtual HRESULT STDMETHODCALLTYPE RunStop()
	{
		Shutdown();
		return S_OK;
	}

	virtual DWORD	_upStartRunService(DWORD argc, LPTSTR *argv)
	{
		UNREFERENCED_PARAMETER(argc);
		UNREFERENCED_PARAMETER(argv);
		return Start();
	}

#ifdef _DEBUG
	virtual DWORD   STDMETHODCALLTYPE RunRun()
	{
		return _upStartRunService(0, 0);
	}
#endif // _DEBUG
};
CMyObjectSolid<CSimpleMain<CMyService>> gOneApp;

BOOL WINAPI HandlerRoutine(__in  DWORD dwCtrlType)
{
	if ( dwCtrlType == CTRL_C_EVENT)
	{
		gOneApp.RunStop();
		return TRUE;
	}
	return FALSE;
}

int _tmain(int argc, _TCHAR* argv[])
{
	gOneApp.ConsoleMain(argc, argv);

	return 0;
}

