#include "stdafx.h"

#include "AppChannel.h"

#include <Locale.h>
#include "CommonInclude/ServiceImpl.h"
#include "CommonInclude/MainImpl.h"


class CMyService : public CCtepCommunicationServer
	, public CRunOneServiceImpl<CMyService>
	, public CServiceImpl<CMyService>
{
public:
	CMyService():CServiceImpl(L"CTEP", L"Cloud Times CTEP Foundation Architecture")
	{
		;
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

int _tmain(int argc, _TCHAR* argv[])
{
	gOneApp.ConsoleMain(argc, argv);

	return 0;
}

