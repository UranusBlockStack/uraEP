

#pragma once

#if _DEBUG
#define DbgPrintf(...)  printf(__VA_ARGS__)
#else
#define DbgPrintf(...)
#endif

#include "CommonInclude/Tools/MoudlesAndPath.h"
#include "../Plugin/CtepTcTcp/CTEPTCTcp.h"

class CLoadModulesClient
{
public:
	CLoadModulesClient():m_TransCltTcp(0), m_TransCltUdp(0){}

	//CLoadModules<ICTEPTransferProtocolClient>	m_TransClt;		// index 0: TCP, 1:UDP
	CLoadModules<ICTEPAppProtocol>				m_APP;

	ICTEPTransferProtocolClient*				m_TransCltTcp;
	ICTEPTransferProtocolClient*				m_TransCltUdp;

public:
	BOOL FindMoudleInterface()
	{
		//m_TransClt.FindMoudleInterface(FUNC_CTEPGetInterfaceTransClient, L"CTEPTC*.dll");
		m_APP.FindMoudleInterface(FUNC_CTEPGetInterfaceAppClient, L"CTEPAC*.dll");
		m_TransCltTcp = CTEPGetInterfaceTransClientTcp();
		ASSERT(m_TransCltTcp);
		if ( m_TransCltTcp)
		{
			return TRUE;
		}
		return FALSE;
	}
};