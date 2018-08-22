#include "stdafx.h"

#include "CommonInclude/RegEdit/RegEdit.cpp"

static LPCWSTR CtepCommClientSubKey = L"Software\\CloudTimes\\CloudTimes Extension Protocol Client";
static LPCWSTR CtepCommClientKeyName = L"Name";
static LPCWSTR CtepCommClientKeyName2 = L"Path";

HRESULT WINAPI DllRegisterServer(void)
{
	CRegistry Reg(HKEY_CURRENT_USER);
	WCHAR path[MAX_PATH];
	BOOL bResult;

	bResult = Reg.CreateKey(CtepCommClientSubKey);
	if ( !bResult) goto End;

	bResult = GetSelfDir(path);
	if ( !bResult) 	goto End;
	bResult = Reg.Write(CtepCommClientKeyName2, path);
	if ( !bResult) goto End;

	DWORD dwFileName = GetModuleFileName(GetSelfModuleHandle(), path, MAX_PATH);
	if ( dwFileName > 0)
	{
		ASSERT(dwFileName < MAX_PATH);
		bResult = Reg.Write(CtepCommClientKeyName, path);
		if ( bResult)
			return S_OK;
	}

End:
	ASSERT(0);
	return SELFREG_E_CLASS;
}

HRESULT WINAPI DllUnregisterServer(void)
{
	CRegistry Reg(HKEY_CURRENT_USER);
	BOOL bRet = Reg.DeleteKey(HKEY_LOCAL_MACHINE, CtepCommClientSubKey);
	if ( !bRet)
		return SELFREG_E_CLASS;

	return S_OK;
}


HRESULT WINAPI DllInstall( BOOL , LPCWSTR )
{
	return S_OK;
}
































