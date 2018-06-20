// CtepTestApp.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"

struct PerUserApp
{
	PerUserApp()
	{
		dwSequenceLocal = dwSequenceRemote = 1;
	}
	DWORD dwSequenceLocal;
	DWORD dwSequenceRemote;
};

class MyTestApp : public ICTEPAppProtocol
{
private:
	Log4CppLib m_log;
	ICTEPAppProtocolCallBack* m_piAPC;

public:
	MyTestApp():m_piAPC(nullptr), m_log("TstApp")
	{
		;
	}

	virtual LPCSTR   GetName() override	{	return "TestApp";}// AppName character count <= 15

	virtual HRESULT  Initialize(ICTEPAppProtocolCallBack* pI) override
	{
		ASSERT(m_piAPC == nullptr);
		m_piAPC = pI;
		return S_OK;
	}
	virtual void	 Final() override	// server close.
	{ASSERT(m_piAPC == nullptr);}

	virtual HRESULT  Connect(   CUserData* pUser, CAppChannel *pNewAppChannel, CAppChannel *pStaticAppChannel/* = nullptr*/)
	{
		pNewAppChannel->pAppParam = new PerUserApp();
		PerUserApp* pUA = (PerUserApp*)pNewAppChannel->pAppParam;

		m_log.print(L"Connect. AppChannelId: %d [%d] Master:%d"
			, pNewAppChannel->AppChannelId, pNewAppChannel->Type, pStaticAppChannel ? pStaticAppChannel->AppChannelId : -1);
		HRESULT hr = m_piAPC->WritePacket(pNewAppChannel, (char*)&pUA->dwSequenceLocal, 4);
		ASSERT(SUCCEEDED(hr));
		pUA->dwSequenceLocal++;
		return S_OK;
	}
	virtual HRESULT  ReadPacket(CUserData* pUser, CAppChannel *pAppChannel, char* pBuff, ULONG size)
	{
		PerUserApp* pUA = (PerUserApp*)pAppChannel->pAppParam;
		if ( !pUA)
			return E_FAIL;

		ASSERT(size == 4);
		ASSERT((*(DWORD*)pBuff) == pUA->dwSequenceRemote);

		m_log.print(L"ReadPacket %d[%d]   data:%d"
			, pAppChannel->AppChannelId, pAppChannel->Type
			, (*(DWORD*)pBuff));
		pUA->dwSequenceRemote++;

		HRESULT hr = m_piAPC->WritePacket(pAppChannel, (char*)&pUA->dwSequenceLocal, 4);
		pUA->dwSequenceLocal++;
		
		if ( pAppChannel->Type == StaticChannel)
		{
			if ( pUA->dwSequenceLocal%100 == 0)
			{
				m_piAPC->CreateDynamicChannel(pAppChannel, Low);
			}
		}
		else
		{
			if ( pUA->dwSequenceLocal > 1500)
			{
				m_piAPC->CloseDynamicChannel(pAppChannel);
			}
		}

		return S_OK;
	}
	virtual void	 Disconnect(CUserData* pUser, CAppChannel *pAppChannel)
	{
		PerUserApp* pUA = (PerUserApp*)pAppChannel->pAppParam;
		delete pUA;
		pAppChannel->pAppParam = nullptr;
	}


}
gOne;

ICTEPAppProtocol* WINAPI CTEPGetInterfaceAppServer()
{
	return (ICTEPAppProtocol*)&gOne;
}

ICTEPAppProtocol* WINAPI CTEPGetInterfaceAppClient()
{
	return (ICTEPAppProtocol*)&gOne;
}

























