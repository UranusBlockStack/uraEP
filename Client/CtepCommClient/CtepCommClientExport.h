#pragma once
#include "Interface\CTEP_Communicate_TransferLayer_Interface.h"

typedef HRESULT (WINAPI *FnCTEPCommClientInitalize)(ICTEPTransferProtocolClient* piTransProtClt, const sockaddr* soa /*= 0*/, int size /*= 0*/);
HRESULT WINAPI CTEPCommClientInitalize(ICTEPTransferProtocolClient* piTransProtClt, const sockaddr* soa = 0, int size = 0);
#define GetFnCTEPCommClientInitalize( mod )		(FnCTEPCommClientInitalize)GetProcAddress(mod, "CTEPCommClientInitalize")

typedef void (WINAPI *FnCTEPCommClientClose)();
void WINAPI CTEPCommClientClose();
#define GetFnCTEPCommClientClose( mod )			(FnCTEPCommClientClose)GetProcAddress(mod, "CTEPCommClientClose")







