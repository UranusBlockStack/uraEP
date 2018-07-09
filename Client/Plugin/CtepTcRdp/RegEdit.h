#pragma once

#include <winreg.h>
#include "CommonInclude/Tools/MoudlesAndPath.h"

class CRegistry
{
	// Construction
public:
	CRegistry(HKEY hKey=HKEY_LOCAL_MACHINE) : m_hKey(hKey){;}
	virtual ~CRegistry(){Close();}

public:
	BOOL SaveKey(LPCTSTR lpFileName);
	BOOL RestoreKey(LPCTSTR lpFileName);
	BOOL Read(LPCTSTR lpValueName, CString* lpVal);
	BOOL Read(LPCTSTR lpValueName, DWORD* pdwVal);
	BOOL Read(LPCTSTR lpValueName, int* pnVal);
	BOOL Write(LPCTSTR lpSubKey, LPCTSTR lpVal);
	BOOL Write(LPCTSTR lpSubKey, DWORD dwVal);
	BOOL Write(LPCTSTR lpSubKey, int nVal);
	BOOL DeleteKey(HKEY hKey, LPCTSTR lpSubKey);
	BOOL DeleteValue(LPCTSTR lpValueName);
	void Close();
	BOOL Open(LPCTSTR lpSubKey);
	BOOL CreateKey(LPCTSTR lpSubKey);

protected:
	HKEY m_hKey;

};



