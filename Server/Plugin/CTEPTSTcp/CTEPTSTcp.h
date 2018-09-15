#ifndef __CTEPTSTCP_H__
#define __CTEPTSTCP_H__
#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>
#pragma comment(lib,"WS2_32.lib")
#include "Interface/CTEP_Communicate_TransferLayer_Interface.h"

#define DEFAULT_PORT              4567
#define MAX_LISTEN_CONNECTION     200

ICTEPTransferProtocolServer* WINAPI CTEPGetInterfaceTransServerTcp();

#endif