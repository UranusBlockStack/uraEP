#include "stdafx.h"
#include "CommonInclude/Regedit/RegEdit.cpp"


LPCWSTR subkey = L"Software\\CT\\CtepTcCtvp";
LPCWSTR keyname = L"Name";

STDAPI DllRegisterServer(void)
{
	HRESULT hr = SELFREG_E_CLASS;
	CRegistry Reg(HKEY_LOCAL_MACHINE);
	WCHAR path[MAX_PATH];

	int dwRet = GetModuleFileName(GetSelfModuleHandle(), path, MAX_PATH);
	ASSERT(dwRet>0);
	if ( dwRet <= 0)
		goto End;

	BOOL bRet;
	bRet = Reg.CreateKey(subkey);
	ASSERT(bRet); 
	if ( !bRet) goto End;

	bRet = Reg.Write(keyname, path);
	ASSERT(bRet);
	if ( !bRet) goto End;

	hr = S_OK;

End:

	return hr;
}

STDAPI DllUnregisterServer(void)
{
	HRESULT hr = SELFREG_E_CLASS;
	CRegistry Reg(HKEY_LOCAL_MACHINE);

	BOOL bRet;
	bRet = Reg.DeleteKey(HKEY_CURRENT_USER, subkey);
	if ( !bRet) goto End;

	hr = S_OK;

End:

	return hr;
}


STDAPI DllInstall( BOOL bInstall, LPCWSTR pszCmdLine)
{
	return S_OK;
}





