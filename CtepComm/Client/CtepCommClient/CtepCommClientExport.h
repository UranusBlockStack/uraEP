#pragma once

typedef HRESULT (*FnCTEPCommClientInitalize)(ICTEPTransferProtocolClient* piTransProtClt, const sockaddr* soa /*= 0*/, int size /*= 0*/);
HRESULT CTEPCommClientInitalize(ICTEPTransferProtocolClient* piTransProtClt, const sockaddr* soa = 0, int size = 0);

typedef void (*FnCTEPCommClientClose)();
void CTEPCommClientClose();








