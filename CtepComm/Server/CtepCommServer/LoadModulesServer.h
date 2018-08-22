
#pragma once

#include "CommonInclude/Tools/MoudlesAndPath.h"

#include "Interface/CTEP_Trans_Packet_Protocol.h"
#include "Interface/CTEP_Common_Struct_Ex.h"

#include "../Plugin/CtepTsTcp/CTEPTSTcp.h"
#include "../Plugin/CTEPTsCrossApp/CTEPTsCross.h"

class CLoadModulesServer
{
public:
	CLoadModulesServer():m_TransSvrTcp(0), m_TransSvrUdp(0), m_TransSvrCrossApp(0){;}

	CLoadModules<ICTEPTransferProtocolServer>	m_TransSvr;		// index 0: TCP, 1:UDP
	CLoadModules<ICTEPAppProtocol>				m_APP;
	
	ICTEPTransferProtocolServer*				m_TransSvrTcp;
	ICTEPTransferProtocolServer*				m_TransSvrUdp;
	ICTEPTransferProtocolServer*				m_TransSvrCrossApp;

	BOOL FindMoudleInterface()
	{
		m_TransSvr.FindMoudleInterface(FUNC_CTEPGetInterfaceTransServer, L"CTEPTS*.dll");
		m_APP.FindMoudleInterface(FUNC_CTEPGetInterfaceAppServer, L"CTEPAS*.dll");
		m_TransSvrTcp = CTEPGetInterfaceTransServerTcp();
		m_TransSvrCrossApp = CTEPGetInterfaceTransServerCrossApp();
		ASSERT(m_TransSvrTcp);
		if ( m_APP._nCount > 0)
		{
			return TRUE;
		}

		return FALSE;
	}
};




