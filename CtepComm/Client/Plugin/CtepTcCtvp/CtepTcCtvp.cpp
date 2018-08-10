// CTEPTStestTcp.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "CtepTcCtvp.h"

#include "CommonInclude/Tools/AdapterInformation.h"

CTVPTransferClient::CTVPTransferClient(PCTVPCHANNEL_ENTRY_POINTS pEntryPoints)
	: CCTVPVirtualChannelManager(pEntryPoints), m_pTransChn(nullptr)
	, m_log("TS_CTVP")
{
#ifdef _DEBUG
	m_log.Message(0xA, L"Debug");
#endif // _DEBUG
	InitializeCriticalSection(&cs);
}
CTVPTransferClient::~CTVPTransferClient()
{
	DeleteCriticalSection(&cs);
}
LPCSTR CTVPTransferClient::GetName()	// 返回传输协议名称
{
	return "CTVP";
}
HRESULT CTVPTransferClient::Initialize(ICTEPTransferProtocolClientCallBack* pI)
{
	ASSERT(!m_piCallBack || m_piCallBack == pI);
	ASSERT(pI);
	m_piCallBack = pI;
	return S_OK;
}
void CTVPTransferClient::Final()
{

}
HRESULT CTVPTransferClient::Connect(CTransferChannel* pTransChn, ReadWritePacket* pPacket /*= 0*/)
{
	HRESULT hr = S_OK;
	ASSERT(m_pTransChn == nullptr);

	pTransChn->hFile = *(GetChannel()->m_phChannel);
	m_pTransChn = pTransChn;

	if ( pPacket)
	{
		hr = Send(pTransChn, pPacket);
	}

	return hr;
}
HRESULT CTVPTransferClient::Disconnect(CTransferChannel* pTransChn)
{
	if ( pTransChn && pTransChn->hFile != INVALID_HANDLE_VALUE)
	{
		pTransChn->hFile = INVALID_HANDLE_VALUE;
		ASSERT(pTransChn == m_pTransChn);
		m_pTransChn = nullptr;
		m_piCallBack->Recv(pTransChn, 0);
	}

	return S_OK;
}

//同步发送数据
HRESULT CTVPTransferClient::Send(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	int ret = WriteDataToChannel(pPacket);

	if( ret != 1)
	{
		return E_FAIL;
	}

	return ERROR_IO_PENDING;
}

HRESULT CTVPTransferClient::Recv(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	return E_NOTIMPL;
}

void CTVPTransferClient::RecvData(char *pbuff, DWORD size)
{
	ASSERT(m_piCallBack);
	static ReadWritePacket* pPacket = m_piCallBack->AllocatePacket(this);
	ASSERT(pPacket);
	pPacket->opType = OP_IocpRecv;
	pPacket->buff.buff = pbuff;
	pPacket->buff.size = size;
	
	m_piCallBack->Recv(m_pTransChn, pPacket);
}

#include "CommonInclude/Tools/MoudlesAndPath.h"
CTVPTransferClient *g_pCtvpTransClient = nullptr;
HMODULE g_hModule;
FnCTEPCommClientInitalize g_fnInit;
FnCTEPCommClientClose	  g_fnClose;

//CTVP通道的默认入口函数
BOOL VCAPITYPE VirtualChannelEntry(PCTVPCHANNEL_ENTRY_POINTS pEntryPoints)
{
	Sleep(8000);
	OutputDebugString(TEXT("SYSINF_C: VirtualChannelEntry\n"));
	g_pCtvpTransClient = new CTVPTransferClient(pEntryPoints);
	ASSERT(g_pCtvpTransClient);
	WCHAR Path[MAX_PATH+1];
	GetSelfDir(Path);
	wcscat_s(Path, L"CtepCommClient.dll");
	g_hModule = LoadLibrary(Path);
	ASSERT(g_hModule);
	if ( g_hModule)
	{
		HRESULT hr = E_FAIL;
		g_fnInit = (FnCTEPCommClientInitalize)GetProcAddress(g_hModule, "CTEPCommClientInitalize");
		g_fnClose = (FnCTEPCommClientClose)GetProcAddress(g_hModule, "CTEPCommClientClose");
		ASSERT(g_fnInit && g_fnClose);
		if ( g_fnInit && g_fnClose)
		{
			return TRUE;
		}
	}
	delete g_pCtvpTransClient;
	g_pCtvpTransClient = nullptr;

	return FALSE;
}

VOID __stdcall CTVPVirtualChannelInitEventProc(LPVOID pInitHandle, UINT event, LPVOID pData, UINT dataLength)
{

	UNREFERENCED_PARAMETER(pInitHandle);
	UNREFERENCED_PARAMETER(dataLength);

	OutputDebugString(TEXT("CTVPVirtualChannelInitEventProc ------------ \n"));
	switch(event)
	{
	case CHANNEL_EVENT_INITIALIZED:
		{
			//   DisplayError(TEXT("SYSINF_C: channel initialized\r\n"));
		}
		break;

	case CHANNEL_EVENT_CONNECTED:
		{
			OutputDebugString(TEXT("CHANNEL_EVENT_CONNECTED ------------ \r\n"));

			//1.当通道初始化以后，获取已经连接的CTVP远程桌面的IP地址
			//2.通过管道给通信层发送一个消息，或者启动通信层
			g_pCtvpTransClient->OpenChannelByInitHandle(pInitHandle);
			if ( g_fnInit)
			{
				g_fnInit(g_pCtvpTransClient, 0, 0);
			}
		}
		break;

	case CHANNEL_EVENT_V1_CONNECTED:
		{
			OutputDebugString(TEXT("CHANNEL_EVENT_V1_CONNECTED ------------ \r\n"));
		}
		break;

	case CHANNEL_EVENT_DISCONNECTED:
		{
			OutputDebugString(TEXT("CHANNEL_EVENT_DISCONNECTED ------------ \r\n"));

			//通过管道给通信层发送一个消息，或者关闭通信层
			g_pCtvpTransClient->CloseChannelByInitHandle(pInitHandle);

			if ( g_fnClose)
			{
				g_fnClose();
			}
		}
		break;

	case CHANNEL_EVENT_TERMINATED:
		{
			OutputDebugString(TEXT("CHANNEL_EVENT_TERMINATED ------------ \r\n"));
			g_pCtvpTransClient->CloseChannelByInitHandle(pInitHandle);
			if ( g_fnClose)
			{
				g_fnClose();
			}

		}
		break;

	default:
		{
			OutputDebugString(TEXT("SYSINF_C: unrecognized init event\r\n"));
		}
		break;
	}
}

void WINAPI CTVPVirtualChannelOpenEvent(DWORD openHandle, UINT event, LPVOID pdata, UINT32 dataLength, UINT32 totalLength, UINT32 dataFlags)
{
	LPDWORD pdwControlCode = (LPDWORD)pdata;

	UNREFERENCED_PARAMETER(openHandle);
	UNREFERENCED_PARAMETER(dataFlags);


	switch(event)
	{
	case CHANNEL_EVENT_READ_COMPLETE:
		{
			ASSERT( g_pCtvpTransClient);
			if ( g_pCtvpTransClient)
			{
				g_pCtvpTransClient->RecvData((char*)pdata, dataLength);
			}
		}
		break;

	case CHANNEL_EVENT_WRITE_COMPLETE:
		{
			if (g_pCtvpTransClient)
			{
				g_pCtvpTransClient->ChannelWriteComplete((ReadWritePacket*)pdata);
			}
		}
		break;

	case CHANNEL_EVENT_WRITE_CANCELLED:
		{
			if ( g_pCtvpTransClient)
			{
				g_pCtvpTransClient->ChannelWriteComplete((ReadWritePacket*)pdata);
			}
		}
		break;

	default:
		{
			OutputDebugString(TEXT("SYSINF_C: unrecognized open event\r\n"));
			//ASSERT(0);
		}
		break;
	}

}