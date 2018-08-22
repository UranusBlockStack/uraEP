#include "stdafx.h"

#include "CommonInclude/RegEdit/RegEdit.cpp"

static LPCWSTR CtepTcIcaSubKey = L"SOFTWARE\\Citrix\\ICA Client\\Engine\\Configuration\\Advanced\\Modules\\ICA 3.0";
static LPCWSTR CtepTcIcaKeyName = L"VirtualDriverEx";		// string:L"CtepTcIca"
static LPCWSTR NewKeyName = L"CtepTcIca";

static LPCWSTR CtepTcIcaSubKey2 = L"SOFTWARE\\Citrix\\ICA Client\\Engine\\Configuration\\Advanced\\Modules\\CtepTcIca";
static LPCWSTR CtepTcIcaKeyName2 = L"DriverName";			// string:L"CtepTcIca.dll"
static LPCWSTR CtepTcIcaKeyName3 = L"DriverNameWin32";		// string:L"CtepTcIca.dll"
static LPCWSTR NewKeyValue = L"CtepTcIca.dll";

HRESULT WINAPI DllRegisterServer(void)
{
	CRegistry Reg(HKEY_LOCAL_MACHINE);
	BOOL bResult;
	CString csFilelist;

	{
		bResult = Reg.Open(CtepTcIcaSubKey);
		if ( !bResult) goto End;

		bResult = Reg.Read(CtepTcIcaKeyName, csFilelist);
		if ( -1 == csFilelist.Find(NewKeyName))
		{
			if ( !csFilelist.IsEmpty())
				csFilelist += L",";

			csFilelist += NewKeyName;

			bResult = Reg.Write(CtepTcIcaKeyName, csFilelist);
			if ( !bResult) goto End;
		}
		Reg.Close();
	}

	{
		CRegistry Reg2(HKEY_LOCAL_MACHINE);
		bResult = Reg2.CreateKey(CtepTcIcaSubKey2);
		if ( !bResult) goto End;

		bResult = Reg2.Write(CtepTcIcaKeyName2, NewKeyValue);
		if ( !bResult) goto End;

		bResult = Reg2.Write(CtepTcIcaKeyName3, NewKeyValue);
		if ( !bResult) goto End;
	}

	return S_OK;
End:
	ASSERT(0);
	return SELFREG_E_CLASS;
}

HRESULT WINAPI DllUnregisterServer(void)
{
	CRegistry Reg(HKEY_LOCAL_MACHINE);
	BOOL bRet = Reg.DeleteKey(HKEY_LOCAL_MACHINE, CtepTcIcaSubKey2);
	if ( !bRet)
		return SELFREG_E_CLASS;

	return S_OK;
}


HRESULT WINAPI DllInstall( BOOL , LPCWSTR )
{
	return S_OK;
}
































