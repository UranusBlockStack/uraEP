#include "StdAfx.h"
#include "VirtualChannelManager.h"

extern VOID VCAPITYPE VirtualChannelInitEventProc(LPVOID pInitHandle, UINT event, LPVOID pData, UINT dataLength);
extern void WINAPI VirtualChannelOpenEvent(DWORD openHandle, UINT event, LPVOID pdata, UINT32 dataLength, UINT32 totalLength, UINT32 dataFlags);

CVirtualChannelManager::CVirtualChannelManager(PCHANNEL_ENTRY_POINTS pEntryPoints):m_Channel(nullptr)
	, m_piCallBack(nullptr)
{
	m_pEntryPoints = (PCHANNEL_ENTRY_POINTS)LocalAlloc(LPTR, pEntryPoints->cbSize);
	ASSERT(m_pEntryPoints);
	memcpy(m_pEntryPoints, pEntryPoints, pEntryPoints->cbSize);
	
	InitChannel();
}
CVirtualChannelManager::~CVirtualChannelManager()
{
	LocalFree(m_pEntryPoints);
}

CVirtualChannelManager::VIRTUAL_CHANNEL_ITEM* CVirtualChannelManager::GetChannel()
{
	return m_Channel;
}

int CVirtualChannelManager::GetTCPLocalProcessInfo(in_addr *IpLocalAddr, in_addr *IpRemoteAddr, DWORD *dwCurPid)
{
	PMIB_TCPTABLE_OWNER_PID pTcpTable;
	DWORD dwSize = 0;

	pTcpTable = (MIB_TCPTABLE_OWNER_PID *) malloc(sizeof (MIB_TCPTABLE_OWNER_PID));
	if (pTcpTable == NULL)
	{
		ASSERT(0);
		return 0;
	}

	//获取本进程PID
	DWORD dwCurrnetPid = GetCurrentProcessId();
	char temp[100] = {"\0"};
	sprintf_s(temp,"通过GetCurrentProcessId获取到的本进程 PID = %d",dwCurrnetPid);
	OutputDebugStringA(temp);

	dwSize = sizeof (MIB_TCPTABLE_OWNER_PID);
	if ( GetExtendedTcpTable(pTcpTable, &dwSize, TRUE,AF_INET,TCP_TABLE_OWNER_PID_ALL,0) ==
		ERROR_INSUFFICIENT_BUFFER) 
	{
		free(pTcpTable);
		pTcpTable = (MIB_TCPTABLE_OWNER_PID *)malloc(dwSize);
		if (pTcpTable == NULL)
		{
			return 0;
		}
	}
	if ( GetExtendedTcpTable(pTcpTable, &dwSize, TRUE,AF_INET,TCP_TABLE_OWNER_PID_ALL,0) == NO_ERROR) 
	{
		for (int i = 0; i < (int) pTcpTable->dwNumEntries; i++)
		{
			char    strState[128]; 
			switch (pTcpTable->table[i].dwState)
			{
			case MIB_TCP_STATE_ESTAB:
				strcpy_s(strState, "ESTAB"); 
				break;
			default:
				break;
			}
			DWORD dwProcessPidTemp1 = ntohs((u_short)pTcpTable->table[i].dwOwningPid);
			unsigned short dwProcessPidTemp = ntohs((unsigned short)(0x0000FFFF & dwProcessPidTemp1));
			if ( dwCurrnetPid != dwProcessPidTemp )
			{
				continue;
			}
			char temp[100] = {"\0"};
			sprintf_s(temp,"通过pTcpTable->table[i].dwOwningPid获取到的本进程 PID = %d",dwProcessPidTemp);
			OutputDebugStringA(temp);
			HANDLE Handle = OpenProcess(
				PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
				FALSE,
				pTcpTable->table[i].dwOwningPid /* This is the PID, you can find one from windows task manager */
				);
			if (Handle) 
			{
				IpLocalAddr->S_un.S_addr = (u_long) pTcpTable->table[i].dwLocalAddr;
				IpRemoteAddr->S_un.S_addr = (u_long) pTcpTable->table[i].dwRemoteAddr;
				*dwCurPid = dwCurrnetPid;
			}
			break;
		}
	} 
	else
	{
		free(pTcpTable);
		return 0;
	}

	if (pTcpTable != NULL) 
	{
		free(pTcpTable);
		pTcpTable = NULL;
	} 
	return 1;   
}



HANDLE CVirtualChannelManager::InitChannel(const char* pChannelName)
{
	DWORD dwChannelNum = 0;
	CHANNEL_DEF cd = {0};
	UINT        uRet;
	LPHANDLE    phChannel;

	ASSERT(!m_Channel);
	strcpy_s(cd.name, pChannelName);
	uRet = m_pEntryPoints->pVirtualChannelInit((LPVOID *)&phChannel,
		(PCHANNEL_DEF)&cd, 1,
		VIRTUAL_CHANNEL_VERSION_WIN2000,
		(PCHANNEL_INIT_EVENT_FN)VirtualChannelInitEventProc);
	if (uRet != CHANNEL_RC_OK)
		goto err_exit;

	if (cd.options != CHANNEL_OPTION_INITIALIZED)
		goto err_exit;
	m_Channel = (VIRTUAL_CHANNEL_ITEM*)malloc(sizeof(VIRTUAL_CHANNEL_ITEM));
	ZeroMemory(m_Channel, sizeof(VIRTUAL_CHANNEL_ITEM));

	m_Channel->m_phChannel = phChannel;
	strcpy_s(m_Channel->m_szChannelName, pChannelName);

	return m_Channel;

err_exit:
	return NULL;
}

void CVirtualChannelManager::OpenChannelByInitHandle( LPVOID pInitHandle)
{
	UINT ui;

	if ( m_Channel && m_Channel->m_phChannel == pInitHandle)
	{
		ui = m_pEntryPoints->pVirtualChannelOpen(m_Channel->m_phChannel,
			&m_Channel->m_dwChannelNum,
			m_Channel->m_szChannelName,
			(PCHANNEL_OPEN_EVENT_FN)VirtualChannelOpenEvent);

		if (ui == CHANNEL_RC_OK)
		{
			m_Channel->m_bChannelOpened = TRUE;
			return ;
		}

		m_Channel->m_bChannelOpened = FALSE;
		OutputDebugString(TEXT("SYSINF_C: virtual channel open failed\r\n"));
	}

	ASSERT(0);
}

//同步发送数据
BOOL CVirtualChannelManager::WriteDataToChannel(ReadWritePacket* pPacket)
{
	ASSERT(m_Channel && pPacket);
	if ( !m_Channel || !pPacket)
		return FALSE;
	m_lckSend.Lock();
	UINT ui = m_pEntryPoints->pVirtualChannelWrite(m_Channel->m_dwChannelNum, pPacket->buff.buff
		, pPacket->buff.size, pPacket);
	ASSERT(ui == CHANNEL_RC_OK);
#ifdef _DEBUG
	CTEPPacket_Header* pHead = (CTEPPacket_Header*)pPacket->buff.buff;
	ASSERT(pHead->magic == 'E');
	ASSERT(pHead->PacketLength == pPacket->buff.size);
	ASSERT(pPacket->buff.size <= CTEP_DEFAULT_BUFFER_SIZE);
	ASSERT(pPacket->opType == OP_IocpSend);

#endif // _DEBUG

	m_lckSend.Unlock();
	
	return ui == CHANNEL_RC_OK;
}

void CVirtualChannelManager::CloseChannelByInitHandle(LPVOID pInitHandle)
{
	ASSERT(m_Channel->m_phChannel == pInitHandle);
	if( m_Channel->m_phChannel == pInitHandle)
	{
		m_Channel->m_bChannelOpened = FALSE;
		m_pEntryPoints->pVirtualChannelClose(m_Channel->m_dwChannelNum);
	}
}
