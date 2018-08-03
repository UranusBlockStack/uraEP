#include "stdafx.h"
#include "RdpTransferClient.h"

CRdpTransferClient::CRdpTransferClient(PCHANNEL_ENTRY_POINTS pEntryPoints)
	: CVirtualChannelManager(pEntryPoints), m_pTransChn(nullptr)
	, m_log("TS_RDP")
{
#ifdef _DEBUG
	m_log.Message(0xA, L"Debug");
#endif // _DEBUG
	InitializeCriticalSection(&cs);
}
CRdpTransferClient::~CRdpTransferClient()
{
	DeleteCriticalSection(&cs);
}
LPCSTR CRdpTransferClient::GetName()	// 返回传输协议名称
{
	return "RDP";
}
HRESULT CRdpTransferClient::Initialize(ICTEPTransferProtocolClientCallBack* pI)
{
	ASSERT(!m_piCallBack || m_piCallBack == pI);
	ASSERT(pI);
	m_piCallBack = pI;
	return S_OK;
}
void CRdpTransferClient::Final()
{

}
HRESULT CRdpTransferClient::Connect(CTransferChannel* pTransChn, ReadWritePacket* pPacket /*= 0*/)
{
	HRESULT hr = S_OK;

	ASSERT(m_pTransChn == nullptr);
	DWORD pid;
	in_addr in_AddrLocal = {0}, in_AddRemote = {0};
	GetTCPLocalProcessInfo(&in_AddrLocal, &in_AddRemote, &pid);

	pTransChn->addrLocal.sin_family = AF_INET;
	pTransChn->addrLocal.sin_addr = in_AddrLocal;
	pTransChn->addrLocal.sin_port = 0;

	pTransChn->addrRemote.sin_family = AF_INET;
	pTransChn->addrRemote.sin_addr = in_AddRemote;
	pTransChn->addrRemote.sin_port = 0;

	pTransChn->hFile = *(GetChannel()->m_phChannel);

	m_pTransChn = pTransChn;

	m_piCallBack->Connected(pTransChn);

	if ( pPacket)
	{
		hr = Send(pTransChn, pPacket);
	}

	return hr;
}
HRESULT CRdpTransferClient::Disconnect(CTransferChannel* pTransChn)
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
HRESULT CRdpTransferClient::Send(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	int ret = WriteDataToChannel(pPacket);

	if( ret != 1)
	{
		return E_FAIL;
	}

	return ERROR_IO_PENDING;
}

HRESULT CRdpTransferClient::Recv(CTransferChannel* pTransChn, ReadWritePacket* pPacket)
{
	return E_NOTIMPL;
}

void CRdpTransferClient::RecvData(char *pbuff, DWORD size)
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
CRdpTransferClient *g_pRdpTransClient = nullptr;
HMODULE g_hModule;
FnCTEPCommClientInitalize g_fnInit;
FnCTEPCommClientClose	  g_fnClose;

//RDP通道的默认入口函数
BOOL VCAPITYPE VirtualChannelEntry(PCHANNEL_ENTRY_POINTS pEntryPoints)
{
	OutputDebugString(TEXT("SYSINF_C: VirtualChannelEntry\n"));
	g_pRdpTransClient = new CRdpTransferClient(pEntryPoints);
	ASSERT(g_pRdpTransClient);

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
	delete g_pRdpTransClient;
	g_pRdpTransClient = nullptr;

	return FALSE;
}

VOID __stdcall VirtualChannelInitEventProc(LPVOID pInitHandle, UINT event, LPVOID pData, UINT dataLength)
{

	UNREFERENCED_PARAMETER(pInitHandle);
	UNREFERENCED_PARAMETER(dataLength);

	OutputDebugString(TEXT("VirtualChannelInitEventProc ------------ \n"));
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

			//1.当通道初始化以后，获取已经连接的RDP远程桌面的IP地址
			//2.通过管道给通信层发送一个消息，或者启动通信层
			g_pRdpTransClient->OpenChannelByInitHandle(pInitHandle);
			if ( g_fnInit)
			{
				g_fnInit(g_pRdpTransClient, 0, 0);
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
			g_pRdpTransClient->CloseChannelByInitHandle(pInitHandle);

			if ( g_fnClose)
			{
				g_fnClose();
			}
		}
		break;

	case CHANNEL_EVENT_TERMINATED:
		{
			OutputDebugString(TEXT("CHANNEL_EVENT_TERMINATED ------------ \r\n"));
			g_pRdpTransClient->CloseChannelByInitHandle(pInitHandle);
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

void WINAPI VirtualChannelOpenEvent(DWORD openHandle, UINT event, LPVOID pdata, UINT32 dataLength, UINT32 totalLength, UINT32 dataFlags)
{
	LPDWORD pdwControlCode = (LPDWORD)pdata;

	UNREFERENCED_PARAMETER(openHandle);
	UNREFERENCED_PARAMETER(dataFlags);


	switch(event)
	{
	case CHANNEL_EVENT_DATA_RECEIVED:
		{
			ASSERT( g_pRdpTransClient);
			if ( g_pRdpTransClient)
			{
				g_pRdpTransClient->RecvData((char*)pdata, dataLength);
			}
		}
		break;

	case CHANNEL_EVENT_WRITE_COMPLETE:
		{
			if (g_pRdpTransClient)
			{
				g_pRdpTransClient->ChannelWriteComplete((ReadWritePacket*)pdata);
			}
		}
		break;

	case CHANNEL_EVENT_WRITE_CANCELLED:
		{
			if ( g_pRdpTransClient)
			{
				g_pRdpTransClient->ChannelWriteComplete((ReadWritePacket*)pdata);
			}
		}
		break;

	default:
		{
			OutputDebugString(TEXT("SYSINF_C: unrecognized open event\r\n"));
			ASSERT(0);
		}
		break;
	}

}