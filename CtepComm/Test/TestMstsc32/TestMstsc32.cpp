// TestMstsc32.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{
	TCHAR path[MAX_PATH] = L"c:\\windows\\syswow64\\mstsc.exe";
// 	STARTUPINFO si;
// 	PROCESS_INFORMATION pi;
// 	BOOL b = CreateProcess(0, path, 0, 0, 0, 0, 0, 0, &si, &pi);
// 	if ( b)
// 	{
// 		CloseHandle(pi.hThread);
// 		CloseHandle(pi.hProcess);
// 	}

	ShellExecute(GetDesktopWindow(), L"open", path, 0, 0, SW_SHOW);

	return 0;
}

