#include "stdafx.h"
#include "RegEdit.h"

LPWSTR subkey = L"Software\\Microsoft\\Terminal Server Client\\Default\\AddIns\\CtepTcRdp";
STDAPI DllRegisterServer(void)
{
	HRESULT hr = SELFREG_E_CLASS;
	CRegistry Reg(HKEY_CURRENT_USER);
	WCHAR path[MAX_PATH];

	int dwRet = GetModuleFileName(GetSelfModuleHandle(), path, MAX_PATH);
	ASSERT(dwRet>0);
	if ( dwRet <= 0)
		goto End;

	BOOL bRet;
	bRet = Reg.CreateKey(subkey);
	if ( !bRet) goto End;

	bRet = Reg.Write(L"Name", path);
	if ( !bRet) goto End;

	hr = S_OK;

End:

	return hr;
}

STDAPI DllUnregisterServer(void)
{
	HRESULT hr = SELFREG_E_CLASS;
	CRegistry Reg(HKEY_CURRENT_USER);

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





