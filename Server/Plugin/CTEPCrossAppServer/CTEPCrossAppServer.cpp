// CTEPCrossAppServer.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"

#include "CommonInclude/DynamicBuffer.h"
#include "CommonInclude/PipeImpl/ClientNamedPipe.h"

#include "Interface/CTEP_Communicate_CrossApp_Interface.h"
#include "Interface/CTEP_Common_Struct_Ex.h"

LPCWSTR g_PipeName = CTEPTS_CROSSAPP_PIPE_NAME_TEMPLATE;


// class FreeBuf
// {
// public:
// 	FreeBuf()
// 	{
// 		ReadWritePacket* pBuffer = new ReadWritePacket();
// 		pBuffer->PacketInit();
// 		m_queFreePacket.Initialize(pBuffer, true, true, 1);
// 		for (int i=0; i<50; i++)
// 		{
// 			pBuffer = new ReadWritePacket();
// 			if ( pBuffer)
// 			{
// 				pBuffer->PacketInit();
// 				m_queFreePacket.Push(pBuffer);
// 			}
// 			else
// 			{
// 				ASSERT(0);
// 			}
// 		}
// 	}
// 
// 	ReadWritePacket* AllocatePacket(HANDLE hFile, EmPacketOperationType opType, DWORD dwSize = 0)
// 	{
// 		ReadWritePacket* p = m_queFreePacket.Pop();
// 		if ( !p)
// 			p = new ReadWritePacket();
// 
// 		ASSERT(dwSize < CTEP_DEFAULT_BUFFER_SIZE);
// 		if ( p)
// 		{
// 			p->PacketInit();
// 			if ( dwSize != 0)
// 				p->buff.size = dwSize;
// 			p->opType = opType;
// 			p->hFile = hFile;
// 		}
// 
// 		return p;
// 	}
// 	void ReleasePacket(ReadWritePacket* pPacket)
// 	{
// 		m_queFreePacket.Push(pPacket);
// 	}
// 
// protected:
// 	FastQueue<ReadWritePacket> m_queFreePacket;
// }
// gFreeBuf;

class CCrossAppTransLayer : private CNamedPipeClient
{
public:
	CCrossAppTransLayer() : CNamedPipeClient(CTEPTS_CROSSAPP_PIPE_NAME_TEMPLATE, sizeof(CTEPPacket_Header))
		, m_log("CrsPipe"), m_ReadBuff(nullptr), m_ReadSize(0)
	{

	}

	HRESULT Connect()
	{
		BOOL bRet = CNamedPipeClient::OpenPipe(FALSE);
		return bRet ? S_OK : E_FAIL;
	}
	void Disconnect()
	{
		CNamedPipeClient::ClosePipe();
	}

	HRESULT PipeSend( __in char* inBuffer, __in DWORD inBufLen = 0)
	{
		BOOL bRet = CNamedPipeClient::WritePipe(inBuffer, inBufLen);
		return bRet ? S_OK : E_FAIL;
	}

	char* m_ReadBuff;
	DWORD m_ReadSize;
	HRESULT PipeRecv(__out char** buff, __out DWORD* dwRead, HANDLE* pHandleEvent = nullptr)
	{
		ASSERT(buff && dwRead);
		*dwRead = 0;
		*buff = nullptr;

		int iRead;
		if ( !pHandleEvent)
			iRead = CNamedPipeClient::ReadPipe((void**)&m_ReadBuff, INFINITE);
		else
		{
			*pHandleEvent = CNamedPipeClient::GetOverlappedReadEvent();
			iRead = CNamedPipeClient::ReadPipe((void**)&m_ReadBuff);
		}

		if ( iRead < 0)
			return E_DISCONNECTED;

		if ( iRead > 0)
		{
			*buff = m_ReadBuff;
			*dwRead = m_ReadSize = iRead;
			return S_OK;
		}

		ASSERT(pHandleEvent);
		return S_IO_PENDING;
	}

	void PipeRecvRelease(__in char* pBuff)
	{
		ASSERT(pBuff == m_ReadBuff);
		CNamedPipeClient::ReleaseRead(m_ReadSize);
		m_ReadBuff = nullptr;
		m_ReadSize = 0;
	}


private:
	_VIRT_D callGetBagSize(BYTE* pRecv, DWORD size) override
	{
		ASSERT(size >= this->m_dwMinBagSize);
#ifdef _DEBUG
		DWORD dw = CTEPPacketGetSize(pRecv);
		ASSERT(dw >= this->m_dwMinBagSize && dw <= CTEP_DEFAULT_BUFFER_SIZE);
#endif // _DEBUG
		return CTEPPacketGetSize(pRecv);	
	}

private:
	HANDLE hFile;
	Log4CppLib m_log;
};


class CCrossAppChannel : public ICTEPAppProtocolCrossCallBack
	, public CAppChannelEx
{
	CCrossAppChannel() : status(none), m_log("CrsAppLnk")
		, m_sAppName(nullptr), m_dwSessionId((DWORD)-1)
		, m_pPacket(nullptr)
		, UserId(INVALID_UID), AppId(INVALID_ACID)
	{
	}

public:
	static CCrossAppChannel* GetInstance()
	{
		return new CCrossAppChannel();
	}

	virtual ~CCrossAppChannel()
	{
		if ( status == working || status == connecting || status == closing)
		{
			Disconnect();
		}
	}

// interface ICTEPAppProtocolCrossCallBack
	_VIRT(CROSSAPP_STATUS) GetStatus() override
	{
		return status;
	}

	_VIRT_H Initialize(LPCSTR sAppName, DWORD SessionId = (DWORD)-1) override		// 初始化获取
	{
		ASSERT(sAppName && SessionId == (DWORD)-1);
		if ( !sAppName || !sAppName[0])
			return E_INVALIDARG;

		LOCK(&lck);
		if ( status != none && status != initialized)
			return E_REFUSE_STATE;

		m_sAppName = sAppName;
		BOOL bRet = ProcessIdToSessionId(GetCurrentProcessId(), &m_dwSessionId);
		if ( !bRet)
		{
			DWORD dwErr = GetLastError();
			ASSERT(0);
			return MAKE_WINDOWS_ERRCODE(dwErr);
		}

		status = initialized;

		return S_OK;
	}
	_VIRT_H Connect() override						//创建一个当前用户当前应用的通道(动态或者静态)
	{
		HRESULT hr = E_FAIL;
		CHAR* buffRead = 0;

		LOCK(&lck);
		if ( status != initialized)
			return E_REFUSE_STATE;

		hr = m_Pipe.Connect();
		if ( FAILED(hr))
		{
			hr = E_DISCONNECTED;
			goto ErrorEnd;
		}

		CHAR  PacketBuf[CTEP_DEFAULT_BUFFER_SIZE];
		DWORD PacketSize;

		CTEPPacket_InitCrossApp* pInitCA = (CTEPPacket_InitCrossApp* )PacketBuf;
		PacketSize = Create_CTEPPacket_InitCrossApp(pInitCA, m_sAppName, m_dwSessionId);
		hr = m_Pipe.PipeSend(PacketBuf, PacketSize);
		if ( FAILED(hr))
		{
			hr = E_REFUSE_APP;
			goto ErrorEnd;
		}

		hr = m_Pipe.PipeRecv(&buffRead, &PacketSize);
		if ( FAILED(hr))
		{
			hr = E_REFUSE_APP;
			goto ErrorEnd;
		}
		
		CTEPPacket_InitCrossApp_Responce* pInitCARsp = (CTEPPacket_InitCrossApp_Responce*)buffRead;
		if ( pInitCARsp->IsPacketInitCrossAppRsp())
		{
			hr = E_UNEXPECTED;
			goto ErrorEnd;
		}
		if ( sizeof(CTEPPacket_InitCrossApp_Responce) != CTEPPacketGetSize(pInitCARsp))
		{
			hr = E_UNEXPECTED;
			goto ErrorEnd;
		}

		if (   pInitCARsp->header.UserId == INVALID_UID
			|| pInitCARsp->header.AppChannelId == INVALID_ACID)
		{
			hr = E_REFUSE_APP;
			goto ErrorEnd;
		}

		AppId = pInitCARsp->header.AppChannelId;
		UserId = pInitCARsp->header.UserId;

		return S_OK;

ErrorEnd:
		if ( buffRead)
		{
			m_Pipe.PipeRecvRelease(buffRead);
		}
		status = error;
		m_Pipe.Disconnect();

		return hr;
	}

	_VIRT_V Disconnect() override
	{
		LOCK(&lck);
		if ( status != working && status != error)
			return ;

		status = closing;
		m_Pipe.Disconnect();
		UserId = INVALID_UID;
		AppId = INVALID_ACID;
		status = initialized;
	}

	_VIRT_H		Write(__in char* buff, __in DWORD size) override
	{
		if ( !buff || size == 0)
			return E_INVALIDARG;

		if ( status != working)
			return E_REFUSE_STATE;

		LOCK(&lckWrite);
		ASSERT( size <= 256*1024);	// 测试是否有超大包

		DWORD nPacketCount = size / CTEP_DEFAULT_BUFFER_DATA_SIZE
			+ (size % CTEP_DEFAULT_BUFFER_DATA_SIZE > 0 ? 1 : 0);

		DWORD dwSizeSent = 0;
		for (DWORD i = 0; i < nPacketCount; i++)
		{
			USHORT dwPacketLength = (USHORT)min(CTEP_DEFAULT_BUFFER_DATA_SIZE, size - dwSizeSent);
			CHAR  PacketBuf[CTEP_DEFAULT_BUFFER_SIZE];
			DWORD PacketSize;

			// 填写头
			if ( nPacketCount == 1)
			{
				PacketSize = Create_CTEPPacket_Message((CTEPPacket_Message*)PacketBuf
					, UserId, AppId, buff, dwPacketLength);
			}
			else
			{
				if ( i == 0)
				{
					PacketSize = Create_CTEPPacket_Message((CTEPPacket_Message*)PacketBuf
						, UserId, AppId, buff+dwSizeSent, dwPacketLength
						, size, true, false);
				}
				else if ( i == nPacketCount - 1)
				{
					PacketSize = Create_CTEPPacket_Message((CTEPPacket_Message*)PacketBuf
						, UserId, AppId, buff+dwSizeSent, dwPacketLength
						, size, false, true);
				}
				else
				{
					PacketSize = Create_CTEPPacket_Message((CTEPPacket_Message*)PacketBuf
						, UserId, AppId, buff+dwSizeSent, dwPacketLength
						, size, false, false);
				}
			}

#ifdef _DEBUG
			DWORD dwPacketHeader = 
				nPacketCount == 1 ? sizeof(CTEPPacket_Header) : sizeof(CTEPPacket_Message);
			ASSERT(PacketSize == dwPacketHeader + dwPacketLength);
#endif // _DEBUG
			
			HRESULT hr = m_Pipe.PipeSend(PacketBuf, PacketSize);
			if ( FAILED(hr))
			{
				ASSERT(0);
				LOCK(&lck);
				if ( status == working)
				{
					status = error;
				}
				return E_FAIL;
			}

			dwSizeSent += dwPacketLength;
		}

		return S_OK;
	}

	char* m_pPacket;
	// Read会返回读到的包, 处理完成后需要调用FreeBuff释放对应内存,赋值pOL会返回S_IO_PENDING, pOL为空时会一直等待到有数据才返回
	_VIRT_H		Read(__out char** outBuff, __out DWORD* outReadSize, HANDLE* pHandleEvent = nullptr) override
	{
		ASSERT(outBuff && outReadSize);
		*outBuff = nullptr;
		*outReadSize = 0;
		if ( status != working)
			return E_REFUSE_STATE;

		HRESULT hr;
		DWORD dwPacketLength;
		LOCK(&lckRead);
		do 
		{
			ASSERT(m_pPacket == nullptr);
			hr = m_Pipe.PipeRecv(&m_pPacket, &dwPacketLength, pHandleEvent);
			if ( S_OK == hr)
			{
				CTEPPacket_Message* pMsg = (CTEPPacket_Message*)m_pPacket;
				ASSERT(m_pPacket && dwPacketLength == CTEPPacketGetSize(pMsg));
				ASSERT(pMsg->IsPacketMessage());
				if ( !pMsg->IsPacketMessage())
				{
					m_Pipe.PipeRecvRelease(m_pPacket);
					hr = E_DISCONNECTED;
					break;
				}

				char* data = CAppChannelEx::SplitPacket(pMsg, *outReadSize);
				if ( data == INVALID_BUFFER)
				{
					m_Pipe.PipeRecvRelease(m_pPacket);
					hr = E_DISCONNECTED;
					break;
				}
				else if ( data == nullptr)
				{
					m_Pipe.PipeRecvRelease(m_pPacket);
					m_pPacket = nullptr;
					continue;
				}
				else
				{
					*outBuff = data;
					break;
				}
			}
			else if ( E_DISCONNECTED == hr)
			{
				break;
			}
			else
			{
				ASSERT( S_IO_PENDING == hr);
				break;
			}
		} while (TRUE);

		if ( FAILED(hr))
		{
			CAppChannelEx::SplitPacketRelease();
			m_pPacket = nullptr;
			LOCK(&lck);
			if ( status == working)
			{
				status = error;
			}
		}

		return hr;
	}

	_VIRT_H FreeBuff( __in char* buff) override
	{
		ASSERT(buff);
		LOCK(&lckRead);
		CAppChannelEx::SplitPacketRelease(buff);
		if ( m_pPacket)
		{
			m_Pipe.PipeRecvRelease(m_pPacket);
			m_pPacket = nullptr;
		}

		return S_OK;
	}

protected:
	Log4CppLib			m_log;

	CCrossAppTransLayer	m_Pipe;

	CROSSAPP_STATUS		status;
	CMyCriticalSection	lck;
	CMyCriticalSection	lckWrite;
	CMyCriticalSection	lckRead;

	LPCSTR				m_sAppName;
	DWORD				m_dwSessionId;
	USHORT				UserId;
	USHORT				AppId;
};



ICTEPAppProtocolCrossCallBack* CrossAppGetInstance()
{
	ICTEPAppProtocolCrossCallBack* pi = CCrossAppChannel::GetInstance();

	return pi;
}

void CrossAppRelease(ICTEPAppProtocolCrossCallBack* pi)
{
	ASSERT(pi);
	if ( pi)
	{
		delete pi;
	}
}



