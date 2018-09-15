#include "StdAfx.h"
#include "VirtualChannelManager.h"

extern VOID VCAPITYPE CTVPVirtualChannelInitEventProc(LPVOID pInitHandle, UINT event, LPVOID pData, UINT dataLength);
extern void WINAPI CTVPVirtualChannelOpenEvent(DWORD openHandle, UINT event, LPVOID pdata, UINT32 dataLength, UINT32 totalLength, UINT32 dataFlags);

CCTVPVirtualChannelManager::CCTVPVirtualChannelManager(PCTVPCHANNEL_ENTRY_POINTS pEntryPoints):m_Channel(nullptr)
	, m_piCallBack(nullptr), m_log("TS_CTVP")
{
	m_pEntryPoints = (PCTVPCHANNEL_ENTRY_POINTS)LocalAlloc(LPTR, pEntryPoints->cbSize);
	ASSERT(m_pEntryPoints);
	memcpy(m_pEntryPoints, pEntryPoints, pEntryPoints->cbSize);

	InitChannel();
}
CCTVPVirtualChannelManager::~CCTVPVirtualChannelManager()
{
	LocalFree(m_pEntryPoints);
}

CCTVPVirtualChannelManager::VIRTUAL_CHANNEL_ITEM* CCTVPVirtualChannelManager::GetChannel()
{
	return m_Channel;
}

HANDLE CCTVPVirtualChannelManager::InitChannel(const char* pChannelName)
{
	DWORD dwChannelNum = 0;
	CTVPCHANNEL_DEF cd = {0};
	UINT        uRet;
	LPHANDLE    phChannel;

	ASSERT(!m_Channel);
	strcpy_s(cd.name, pChannelName);
#ifdef _DEBUG
	DWORD dwTimeDelay = GetTickCount();
#endif // _DEBUG
	uRet = m_pEntryPoints->pVirtualChannelInit((LPVOID *)&phChannel,
		(PCTVPCHANNEL_DEF)&cd, 1,
		VIRTUAL_CHANNEL_VERSION_WIN2000,
		(PCTVPCHANNEL_INIT_EVENT_FN)CTVPVirtualChannelInitEventProc);
#ifdef _DEBUG
	dwTimeDelay = GetTickCount() - dwTimeDelay;
	m_log.FmtMessage(1, L"InitChannel - pVirtualChannelInit: time elapse:%d", dwTimeDelay);
#endif // _DEBUG
	if (uRet != CHANNEL_RC_OK)
		goto err_exit;
	m_Channel = (VIRTUAL_CHANNEL_ITEM*)malloc(sizeof(VIRTUAL_CHANNEL_ITEM));
	ZeroMemory(m_Channel, sizeof(VIRTUAL_CHANNEL_ITEM));

	m_Channel->m_phChannel = phChannel;
	strcpy_s(m_Channel->m_szChannelName, pChannelName);

	return m_Channel;

err_exit:
	return NULL;
}

void CCTVPVirtualChannelManager::OpenChannelByInitHandle( LPVOID pInitHandle)
{
	UINT ui;
	m_Channel->m_phChannel = (LPHANDLE)pInitHandle;
	if ( m_Channel && m_Channel->m_phChannel == pInitHandle)
	{
		ui = m_pEntryPoints->pVirtualChannelOpen(m_Channel->m_phChannel,
			&m_Channel->m_dwChannelNum,
			m_Channel->m_szChannelName,
			(PCTVPCHANNEL_OPEN_EVENT_FN)CTVPVirtualChannelOpenEvent);

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
BOOL CCTVPVirtualChannelManager::WriteDataToChannel(ReadWritePacket* pPacket)
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

void CCTVPVirtualChannelManager::CloseChannelByInitHandle(LPVOID pInitHandle)
{
	ASSERT(m_Channel->m_phChannel == pInitHandle);
	if( m_Channel->m_phChannel == pInitHandle)
	{
		m_Channel->m_bChannelOpened = FALSE;
		m_pEntryPoints->pVirtualChannelClose(m_Channel->m_dwChannelNum);
	}
}
