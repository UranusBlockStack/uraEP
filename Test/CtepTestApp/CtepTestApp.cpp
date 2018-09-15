// CtepTestApp.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"

struct PerUserApp
{
	PerUserApp()
	{
		dwSequenceLocal = dwSequenceRemote = 1;
	}

	DWORD dwSequenceRemote;
	union
	{
		DWORD dwSequenceLocal;
		char buff[32*1024 + 20];
	};
};

struct PerBag
{
	DWORD dwSequenceLocal;
	DWORD dwBagSize;
#pragma warning(suppress:4200) // 
	char  buf[0];

	DWORD SetSize(DWORD bagSize)
	{
		this->dwBagSize = bagSize;
		char *p1 = buf;
		char *p2 = buf + bagSize;
		UCHAR i = (UCHAR)bagSize;
		while(p1 < p2)
		{
			*p1 = i++;
			p1++;
		}

		buf[bagSize-4] = 0;
		buf[bagSize-3] = 0;
		buf[bagSize-2] = 0;
		buf[bagSize-1] = 0;
		return bagSize + sizeof(PerBag);
	}
};

inline int GetRaidNormal()
{
	DWORD dwSizeSend;
	dwSizeSend = rand() + 5;		//1
	if ( dwSizeSend > 4*1024)
		dwSizeSend = rand() + 5;		//2
	if ( dwSizeSend > 4*1024)
		dwSizeSend = rand() + 5;		//3
	if ( dwSizeSend > 4*1024)
		dwSizeSend = rand() + 5;		//4
	if ( dwSizeSend > 4*1024)
		dwSizeSend = rand() + 5;		
	if ( dwSizeSend > 4*1024)
		dwSizeSend = rand() + 5;		
	if ( dwSizeSend > 4*1024)
		dwSizeSend = rand() + 5;		
	if ( dwSizeSend > 4*1024)
		dwSizeSend = rand() + 5;		
	if ( dwSizeSend > 4*1024)
		dwSizeSend = rand() + 5;		
	if ( dwSizeSend > 4*1024)
		dwSizeSend = rand() + 5;		
	if ( dwSizeSend > 4*1024)
		dwSizeSend = rand() + 5;		//11
	return dwSizeSend;
}


DWORD g_dwType;			// 记录工作在服务器模式还是客户端模式
enum CTEPAPPTEST_WORKMODE
{
	UploadApp,
	DownloadApp,
	random,
	extreme,
};

CTEPAPPTEST_WORKMODE mode = extreme;

int GetRaid()
{
	if ( mode == random)
	{
		return GetRaidNormal();
	}
	if ( mode == extreme)
	{
		return rand()+ 5;
	}

	if ( g_dwType & CTEP_TYPE_APP_SERVER)
	{
		if ( mode == DownloadApp)
			return rand()+ 5;
	}
	else
	{
		if ( mode == UploadApp)
			return rand() + 5;
	}

	return 256;
}


class MyTestApp : public ICTEPAppProtocolEx
{
private:
	Log4CppLib m_log;
	ICTEPAppProtocolCallBack* m_piAPC;

public:
	MyTestApp():m_piAPC(nullptr), m_log("TstApp")	{}

	_VIRT(LPCSTR)   GetName() override	{	return "TestApp";}// AppName character count <= 15
	_VIRT_D    GetInterfaceVersion() override {return 1;} //默认返回0, ICTEPAppProtocolEx接口返回1,
	
	_VIRT_D    GetNameCount() override {return 1;}
	_VIRT(LPCSTR)   GetNameIndex(long iIndex = 0) override	// AppName character count <= 15, 用于Proxy支持多个App的代理使用
	{
		if ( iIndex == 0)return GetName();
		return "";
	}

	//Comm模块询问App模块指定静态通道是否可以被关闭了,可以返回TRUE, 拒绝关闭返回FALSE
	_VIRT_B     QueryDisconnect(CUserData* pUser, CAppChannel *pAppChannel) override
	{
		return TRUE;
	}
	
	//Comm模块通知App模块指定通道的状态发生改变(Connected, Disconnected)
	_VIRT_V     ChannelStateChanged(CUserData* pUser, CAppChannel *pAppChannel) override
	{
		;
	}

	_VIRT_H  Initialize(ICTEPAppProtocolCallBack* pI, DWORD dwType) override
	{
		g_dwType = dwType;
		srand(GetTickCount());
		ASSERT(m_piAPC == nullptr);
		m_piAPC = pI;
		return S_OK;
	}
	_VIRT_V	 Final() override	// server close.
	{
		ASSERT(m_piAPC != nullptr);
		m_piAPC = nullptr;
	}

	_VIRT_B     ConnectCrossApp(CUserData* pUser, CAppChannel* pNewAppChannel
		, CAppChannel *pStaticAppChannel) override
	{
		return FALSE;
	}

	_VIRT_H  Connect(   CUserData* pUser, CAppChannel *pNewAppChannel
		, CAppChannel *pStaticAppChannel/* = nullptr*/) override
	{
		pNewAppChannel->pAppParam = new PerUserApp();
		PerUserApp* pUA = (PerUserApp*)pNewAppChannel->pAppParam;

		m_log.FmtMessage(5, L"Connect. AppChannelId: %d [%d] Master:%d"
			, pNewAppChannel->AppChannelId, pNewAppChannel->Type, pStaticAppChannel ? pStaticAppChannel->AppChannelId : -1);

		if ( !(g_dwType&CTEP_TYPE_APP_SERVER) || mode == extreme)
		{
			DWORD dwSizeSend = GetRaid();
			PerBag *pBag = (PerBag *)pUA->buff;
			dwSizeSend = pBag->SetSize(dwSizeSend);

			m_log.print(L"Connect-SendPacket %d[%d]   data:%d size:%d", pNewAppChannel->AppChannelId, pNewAppChannel->Type, pBag->dwSequenceLocal, dwSizeSend);
			HRESULT hr = m_piAPC->WritePacket(pNewAppChannel, (char*)pBag, dwSizeSend);
			ASSERT(SUCCEEDED(hr) || pNewAppChannel->bClosing);
			pUA->dwSequenceLocal++;
		}
		return S_OK;
	}
	_VIRT_H  ReadPacket(CUserData* pUser, CAppChannel *pAppChannel
		, char* pBuff, ULONG size) override
	{
		PerUserApp* pUA = (PerUserApp*)pAppChannel->pAppParam;
		PerBag *pBagRecv = (PerBag *)pBuff;

		if ( !pUA)
			return E_FAIL;

		m_log.print(L"ReadPacket %d[%d] data:%d size:%d"
			, pAppChannel->AppChannelId, pAppChannel->Type, pBagRecv->dwSequenceLocal, size);
		ASSERT(size >= 4);
		ASSERT(pBagRecv->dwBagSize + sizeof(PerBag) == size);
		ASSERT(pBagRecv->buf[pBagRecv->dwBagSize-4] == 0);
		ASSERT(pBagRecv->buf[pBagRecv->dwBagSize-3] == 0);
		ASSERT(pBagRecv->buf[pBagRecv->dwBagSize-2] == 0);
		ASSERT(pBagRecv->buf[pBagRecv->dwBagSize-1] == 0);
		UCHAR cBase = (UCHAR)pBagRecv->dwBagSize;
		for (DWORD i = 1; i < pBagRecv->dwBagSize-4; i += min(256, i))
		{
			UCHAR iCount = (UCHAR)i;
			ASSERT((UCHAR)(iCount + cBase) == (UCHAR)pBagRecv->buf[i]);
		}
		ASSERT(pBagRecv->dwSequenceLocal == pUA->dwSequenceRemote);

		pUA->dwSequenceRemote++;
		DWORD dwSizeSend = GetRaid();
		PerBag *pBag;
		if ( dwSizeSend + sizeof(PerBag) > CTEP_DEFAULT_BUFFER_DATA_SIZE)
		{
			pBag = (PerBag *)pUA->buff;
			dwSizeSend = pBag->SetSize(dwSizeSend);
			m_log.print(L"SendPacket %d[%d]   data:%d size:%d"
				, pAppChannel->AppChannelId, pAppChannel->Type
				, pBag->dwSequenceLocal, dwSizeSend);
			HRESULT hr = m_piAPC->WritePacket(pAppChannel, (char*)pBag, dwSizeSend);
		}
		else
		{
			ReadWritePacket* pBuffer = m_piAPC->MallocSendPacket(pAppChannel, (USHORT)dwSizeSend + sizeof(PerBag));
			pBag = (PerBag *)pBuffer->pointer;
			pBag->dwSequenceLocal = pUA->dwSequenceLocal;
			dwSizeSend = pBag->SetSize(dwSizeSend);
			m_log.print(L"SendPacket %d[%d]   data:%d size:%d"
				, pAppChannel->AppChannelId, pAppChannel->Type
				, pBag->dwSequenceLocal, dwSizeSend);
			HRESULT hr = m_piAPC->WritePacket(pAppChannel, pBuffer);
		}

		pUA->dwSequenceLocal++;
		
		if ( pAppChannel->Type == StaticChannel && pAppChannel->nCount < 10)
		{
			if ( pUA->dwSequenceLocal % 10000 == 0)
			{
				m_piAPC->CreateDynamicChannel(pAppChannel, Low);
			}
		}
		else if ( pAppChannel->Type == DynamicChannel || pAppChannel->Type == CrossDyncChannel)
		{
			if ( pUA->dwSequenceLocal > 20000)
			{
				m_piAPC->CloseDynamicChannel(pAppChannel);
			}
		}

		return S_OK;
	}
	_VIRT_V	 Disconnect(CUserData* pUser, CAppChannel *pAppChannel) override
	{
		m_log.FmtMessage(5, L"Disconnect App. AppId:%d[%s]", pAppChannel->AppChannelId, pAppChannel->debugType());

		PerUserApp* pUA = (PerUserApp*)pAppChannel->pAppParam;
		pAppChannel->pAppParam = nullptr;
		ASSERT(pUA->dwSequenceLocal != 0xdddddddd);
		delete pUA;
		
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

























