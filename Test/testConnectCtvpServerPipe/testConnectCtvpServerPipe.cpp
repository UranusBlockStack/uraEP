// testConnectCtvpServerPipe.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "CommonInclude/Tools/ProcessThread.h"

#include "../../Server/Plugin/CTEPTsCtvp/CTEPTsCtvp.h"

Log4CppLib m_log("Ctvp");

void PrintTime()
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	m_log.FmtMessage(7, L"Date:%d.%d.%d	    	%2d:%02d:%02d"
			, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}

int _tmain(int argc, _TCHAR* argv[])
{
	DWORD dwError;
	HANDLE m_hPipeWaitConnect;
	CThreadCommon<> DoWait;
	PrintTime();

	while(TRUE)
	{
		DWORD dwConsoleId = WTSGetActiveConsoleSessionId();
		if ( dwConsoleId == (DWORD)-1)
		{
			//SleepEx(1000, TRUE);
			//continue;
			dwConsoleId = 0;
		}
		WCHAR PipeName[MAX_PATH];
		swprintf_s(PipeName, CTEPTS_CTVP_PIPE_NAME_TEMPLATE, dwConsoleId);
		m_log.FmtMessage(7, L"WaitNamedPipe start. PipeName:%s", PipeName);

		BOOL bWait = WaitNamedPipe(PipeName, 10000);
		dwError = GetLastError();
		m_log.FmtMessage(7, L"WaitNamedPipe return. bWait:%d, ErrCode:%d"
			, bWait, dwError);
		if ( !bWait)
		{
			m_log.FmtWarning(1, L" WaitNamedPipe() failed. Pipe:[%s] ErrCode:%d"
				, PipeName, dwError);
		}
		if ( !bWait)
		{
			Sleep(1000);
			continue;
		}

		m_hPipeWaitConnect = CreateFile(PipeName,
			GENERIC_READ|GENERIC_WRITE, 
			0,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED,
			NULL);
		dwError = GetLastError();
		if ( m_hPipeWaitConnect  == INVALID_HANDLE_VALUE)
		{
			ASSERT(dwError == ERROR_PIPE_BUSY);
			m_log.FmtError(1, L"CreateFile() failed. Pipe:[%s] ErrCode:%d"
				, PipeName, dwError);
			continue;
		}

		m_log.FmtMessage(2, L"CreateFile() New Link in. handle:0x%08x, pipe:[%s]"
			, m_hPipeWaitConnect, PipeName);

		// 获取第一个包,确认用户已经连入了
		CHAR buff[MAX_PATH];
		DWORD dwRead = 0;
		OVERLAPPED ol = {0};
		ol.hEvent = CreateEvent(0, TRUE, FALSE, 0);

		for (;;)
		{
			dwRead = 0;
			BOOL bRead = ReadFile(m_hPipeWaitConnect, buff, MAX_PATH, &dwRead, &ol);
			dwError = GetLastError();
			if ( bRead || dwError == ERROR_IO_PENDING)
			{
				m_log.FmtMessage(7, L"ReadFile Post. Return:%d, ReadSize:%d ErrCode:%d==997(ERROR_IO_PENDING)"
					, bRead, dwRead, dwError);
				HANDLE hGroup[] = {ol.hEvent};
				DWORD dwWait = DoWait.MsgWait(hGroup, 1, INFINITE);
				m_log.FmtMessage(7, L"MsgWait Return, dwWait:%d == 0", dwWait);
				bRead = GetOverlappedResult(m_hPipeWaitConnect, &ol, &dwRead, FALSE);
				dwError = GetLastError();

				if ( dwError == ERROR_IO_INCOMPLETE)
				{
					ASSERT(dwWait == 0);
				}
			}

			if ( !bRead || dwRead == 0)
			{
				m_log.FmtError(2, L"GetOverlappedResult ReadFile() failed. handle:0x%08x BOOL:%d ReadSize:%d ErrCode:%d"
					, m_hPipeWaitConnect, bRead, dwRead, dwError);
				ASSERT(dwError == ERROR_PIPE_NOT_CONNECTED	// 管道被关闭
					|| dwError == ERROR_BROKEN_PIPE			// 管道被关闭
					|| dwError == ERROR_IO_INCOMPLETE);
				CloseHandle(m_hPipeWaitConnect);
				break;
			}

			m_log.FmtMessage(7, L"GetOverlappedResult return, handle:0x%08x, Data:%d", m_hPipeWaitConnect, dwRead);
			m_log.DataMessage(5, L"READ", (BYTE*)buff, dwRead);
		}


	}

	return 0;
}

