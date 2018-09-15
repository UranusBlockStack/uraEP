// CtepTestCrossApp.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include "interface/CTEP_Communicate_CrossApp_Interface.h"

LPCSTR gAppName = "TestApp";
Log4CppLib g_log("TstCrsApp");

ICTEPAppProtocolCrossCallBack* giAPC;


int _tmain(int argc, _TCHAR* argv[])
{
	HRESULT hr;
	giAPC = CrossAppGetInstance();
	ASSERT(giAPC);
	if ( !giAPC)
	{
		g_log.Error(5, L"CrossAppGetInstance() failed!!!");
		return -1;
	}
	hr = giAPC->Initialize(gAppName);
	ASSERT(SUCCEEDED(hr));

	hr = giAPC->Connect();
	if ( FAILED(hr))
	{
		g_log.FmtError(5, L"Connect() failed. ErrCode:0x%08x", hr);
		goto End;
	}


End:
	system("pause");
	CrossAppRelease(giAPC);
	return 0;
}

