// CtepAsAppProxy.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"


#include "CtepAsAppProxy.h"
#include "CommonInclude/IniFile/Cipher.cpp"
#include "CommonInclude/IniFile/IniFile.cpp"
#include "CommonInclude/PipeImpl/ServerNamedPipe.cpp"

CCtepAsAppProxy g_One;

ICTEPAppProtocol* WINAPI CTEPGetInterfaceAppServer()
{
	return (ICTEPAppProtocol*)&g_One;
}




