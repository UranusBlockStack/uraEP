// TestWindowsFirewallException.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "WindowsFirewallException.h"



int __cdecl wmain(int argc, wchar_t* argv[])
{
	HRESULT hr = S_OK;
	CWindowsFirewallExcept wf;

	// Retrieve the firewall profile currently in effect.
	hr = wf.Initialize();
	if (FAILED(hr))
	{
		printf("WindowsFirewallInitialize failed: 0x%08lx\n", hr);
		goto error;
	}
	// Turn off the firewall.
	hr = wf.TurnOff();
	if (FAILED(hr))
	{
		printf("WindowsFirewallTurnOff failed: 0x%08lx\n", hr);
		goto error;
	}
	// Turn on the firewall.
	hr = wf.TurnOn();
	if (FAILED(hr))
	{
		printf("WindowsFirewallTurnOn failed: 0x%08lx\n", hr);
		goto error;
	}
	// Add Windows Messenger to the authorized application collection.
	hr = wf.AddApp(
		L"%ProgramFiles%\\Messenger\\msmsgs.exe",
		L"Windows Messenger"
		);
	if (FAILED(hr))
	{
		printf("WindowsFirewallAddApp failed: 0x%08lx\n", hr);
		goto error;
	}
	// Add TCP::80 to list of globally open ports.
	hr = wf.PortAdd(80, NET_FW_IP_PROTOCOL_TCP, L"WWW");
	if (FAILED(hr))
	{
		printf("WindowsFirewallPortAdd failed: 0x%08lx\n", hr);
		goto error;
	}

error:
	// Release the firewall profile.
	wf.Cleanup();
	return 0;
}


