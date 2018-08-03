#pragma once


#include "Interface/CTEP_Communicate_App_Interface.h"

#include "CommonInclude/Tools/LockedContainer.h"
#include "CommonInclude/IniFile/IniFile.h"
#include "CommonInclude/PipeImpl/ServerNamedPipe.h"

#define CTEPASAPPPROXY_MAX_APPCOUNT		50
class CCtepAsAppProxy : public ICTEPAppProtocol, protected CSvrNamedPipe
{
public:
	CCtepAsAppProxy() : CSvrNamedPipe(L"\\\\.\\pipe\\CtepAsAppProxy", FALSE, CallGetBagSize
		, sizeof(CTEPPacket_Header), CTEP_DEFAULT_BUFFER_SIZE)
		, m_nAppNamesCount(0), m_pAppNamesBuff(NULL), m_log("AsProxy")
		, m_piAPCallBack(NULL), m_hTrdDoPipe(NULL)
	{
		m_hEvtQuit = CreateEvent(0, TRUE, TRUE, 0);
		ASSERT(m_hEvtQuit);
		m_dwMinBagSize = sizeof(CTEPPacket_Header);

		ZeroMemory(m_sAppNames, sizeof(m_sAppNames));
		CIniFile ini(L"CtepAsAppProxy.ini");
		CString names = ini.GetKeyValue(L"AppNames", L"NameCollection");
		if ( !names.IsEmpty())
		{
			CStringA strNames(names);

			m_pAppNamesBuff = (LPSTR)malloc(strNames.GetLength()+1);
			strcpy_s(m_pAppNamesBuff, strNames.GetLength()+1, strNames.GetString());
			LPSTR p1 = m_pAppNamesBuff, p2 = NULL;
			while ( p1)
			{
				p2 = strchr(p1, '|');
				if ( p2)
				{
					*p2 = NULL;
					p2++;
				}
				ASSERT(strlen(p1) <= 15 && strlen(p1) > 0);
				m_sAppNames[m_nAppNamesCount++] = p1;
				p1 = p2;
			}
		}
	}
	virtual ~CCtepAsAppProxy()
	{
		CloseHandle(m_hEvtQuit);
		free(m_pAppNamesBuff);
	}

public://interface CSvrNamedPipe
	static DWORD CallGetBagSize(char* pRecv, DWORD dwSize, void*)
	{
		ASSERT(dwSize >= 4 && pRecv);
		if ( dwSize < 4)
			return 0;

		DWORD dwResult;
		CTEPPacket_Header* pHeader = (CTEPPacket_Header*)pRecv;
		dwResult = pHeader->PacketLength;
		ASSERT(pHeader->magic == 'E');
		ASSERT(dwResult >= 4 && dwResult <= 64*1024); //包大小必须大于4, 小于等于64kb
		return dwResult;
	}

	virtual HRESULT evtPipeConnected(PipeNode* pPN) override	// 初始化失败返回hr, 则管道关闭
	{
		HRESULT hr = E_FAIL;


		return hr;
	}
	virtual void evtPipeReadOneBag(PipeNode* pPN, void* bag, DWORD bagSize) override //使用这个函数需要实现GetBagSize()函数
	{
		CTEPPacket_Header* pHeader = (CTEPPacket_Header*)bag;
		ASSERT(bagSize == pHeader->PacketLength);




	}
	virtual void evtPipeClose(PipeNode* pPN) override
	{

	}


public://interface ICTEPAppProtocol
	virtual LPCSTR   GetName() override {return NULL;}
	virtual HRESULT  Initialize(ICTEPAppProtocolCallBack* pI, DWORD dwType) override
	{
		ASSERT(dwType == CTEP_TYPE_APP_SERVER);
		ASSERT(!m_piAPCallBack || m_piAPCallBack == pI);

		if ( m_nAppNamesCount == 0)
			return E_NOTIMPL;

		if ( m_State != svr_stoped)
		{
			ASSERT(0);
			if ( m_State == svr_started)
				return S_FALSE;

			return E_NOINTERFACE;
		}

		HRESULT hr = E_FAIL;
		m_piAPCallBack = pI;
		ResetEvent(m_hEvtQuit);

		if ( !this->Start())
		{
			m_log.Error(5, L"Initialize - Start() failed.");
			hr = E_UNEXPECTED;
			goto End;
		}

		// 启动管道处理线程
		ASSERT(!m_hTrdDoPipe);
		m_hTrdDoPipe = CreateThread(0, 0, _trdDoPipe, this, 0, 0);
		ASSERT(m_hTrdDoPipe);
		if ( !m_hTrdDoPipe)
			goto End;

		hr = S_OK;
	End:
		if ( FAILED(hr))
		{
			Final();
		}

		return hr;
	}
	virtual void	 Final() override
	{
		SetEvent(m_hEvtQuit);
		WaitForSingleObject(m_hTrdDoPipe, INFINITE);
		m_hTrdDoPipe = NULL;
	}

	virtual HRESULT  Connect(CUserData* pUser, CAppChannel* pNewAppChannel, CAppChannel *pStaticAppChannel/* = nullptr*/) override
	{
		HRESULT hr = E_FAIL;

		return hr;
	}
	virtual HRESULT  ReadPacket(CUserData* pUser, CAppChannel *pAppChannel, char* pBuff, ULONG size) override
	{
		HRESULT hr = E_FAIL;

		return hr;
	}
	virtual void	 Disconnect(CUserData* pUser, CAppChannel *pAppChannel) override
	{

	}

	virtual DWORD    GetNameCount() override
	{
		return m_nAppNamesCount;
	}

	virtual LPCSTR   GetNameIndex(long iIndex) override
	{
		if ( iIndex < 0 || (DWORD)iIndex >= m_nAppNamesCount || m_nAppNamesCount == 0)
		{
			ASSERT(0);
			return NULL;
		}
		return m_sAppNames[iIndex];
	}

private:
	HANDLE			m_hTrdDoPipe;
	static DWORD WINAPI _trdDoPipe(LPVOID This){return ((CCtepAsAppProxy*)This)->TrdDoPipe();}
	DWORD TrdDoPipe()// 管道消息处理线程, 处理管道 read/listen 消息
	{
		int i = 0;
		while ( i == 0)
		{
			i = DoWaitForIO(m_hEvtQuit);
		}
		ASSERT(i == -10);

		Stop();
		return 0;
	}


private:
	HANDLE			m_hEvtQuit;
	LPSTR			m_pAppNamesBuff;
	LPCSTR			m_sAppNames[CTEPASAPPPROXY_MAX_APPCOUNT];
	DWORD			m_nAppNamesCount;
	Log4CppLib		m_log;

	ICTEPAppProtocolCallBack *m_piAPCallBack;


};









