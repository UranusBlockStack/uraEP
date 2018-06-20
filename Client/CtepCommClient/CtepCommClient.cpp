// CtepCommClient.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"

#include "CommonInclude/Tools/FastQueue.h"


#include "Interface/CTEP_Communicate_App_Interface.h"
#include "Interface/CTEP_Common_Struct_Ex.h"

#include "LoadModules.h"

#include "CtepCommClient.h"

CCtepCommClientApp gOne;

HRESULT CTEPCommClientInitalize(ICTEPTransferProtocolClient* piTransProtClt, const sockaddr* soa, int size)
{
	return gOne.Initalize(piTransProtClt, soa, size);
}
