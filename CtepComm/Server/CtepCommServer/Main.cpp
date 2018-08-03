#include "stdafx.h"

#include "AppChannel.h"

#include <Locale.h>
#include "CommonInclude/ServiceImpl.h"
#include "CommonInclude/MainImpl.h"
#include "CommonInclude/Tools/MoudlesAndPath.h"

#include "CommonInclude/Tools/FirewallConfig.h"
#include "CommonInclude/Tools/FirewallConfig.cpp"

BOOL WINAPI HandlerRoutine(__in  DWORD dwCtrlType);

class CMyService : public CCtepCommunicationServer
	, public CRunOneServiceImpl<CMyService>
	, public CServiceImpl<CMyService>
{
public:
	CMyService():CServiceImpl(L"CTEP", L"Cloud Times CTEP Foundation Architecture")
	{
#ifdef _DEBUG
		BOOL bRet = SetConsoleCtrlHandler(HandlerRoutine, TRUE);
		ASSERT(bRet);
#endif // _DEBUG
	}

public:
	virtual HRESULT STDMETHODCALLTYPE RunStop() override
	{
		Shutdown();
		return S_OK;
	}

	virtual DWORD	_upStartRunService(DWORD argc, LPTSTR *argv) override
	{
		UNREFERENCED_PARAMETER(argc);
		UNREFERENCED_PARAMETER(argv);
		return Start();
	}

#ifdef _DEBUG
	virtual DWORD   STDMETHODCALLTYPE RunRun() override
	{
		return _upStartRunService(0, 0);
	}
#endif // _DEBUG

	// -Install
	virtual HRESULT STDMETHODCALLTYPE InstallService(DWORD dwArgc, LPTSTR *lpszArgv) override
	{
		HRESULT hr = __super::InstallService(dwArgc, lpszArgv);
		if ( SUCCEEDED(hr))
		{
			WCHAR path[MAX_PATH] = {0};
			GetSelfDir(path);

			// Ìí¼Ó·À»ðÇ½
			CFireWallAdd::FireWallWindows_Add(path, L"CTEP Foundation Architecture");
		}

		return hr;
	}

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

