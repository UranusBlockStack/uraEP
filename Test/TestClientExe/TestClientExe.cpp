// TestClientExe.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <winsock2.h>
#pragma comment(lib, "Ws2_32")

#include "CommonInclude/Tools/FastQueue.h"


#include "Interface/CTEP_Communicate_App_Interface.h"
#include "Interface/CTEP_Common_Struct_Ex.h"

#include "../../Client/CtepCommClient/LoadModules.h"

#include "../../Client/CtepCommClient/CtepCommClientExport.h"

int main(int argc, CHAR* argv[])
{
	CLoadModules load;
	load.FindMoudleInterface(CLoadModules::CtepAppClient);
	load.FindMoudleInterface(CLoadModules::CtepTransClient);
	ASSERT( load.m_AppCount>0 && load.m_TransCount>0 && load.m_TC[0]);

	SOCKADDR_IN sock = {AF_INET, ::htons(4567), {127,0,0,1}};
	if ( argc > 1)
	{
		sock.sin_addr.S_un.S_addr = ::inet_addr(argv[1]);
	}

	CTEPCommClientInitalize(load.m_TC[0], (sockaddr*)&sock, sizeof(sock));

	return 0;
}

