#pragma once

#include <netfw.h>
#include "CommonInclude/CommonImpl.h"

// #include <objbase.h>
// #include <oleauto.h>
// #pragma comment(lib, "ole32.lib" )
// #pragma comment(lib, "oleaut32.lib")

class CWindowsFirewallExcept
{
	INetFwProfile* fwProfile;
	Log4CppLib m_log;

public:
	CWindowsFirewallExcept():fwProfile(NULL), m_log("Winfire")
	{
		;
	}
	~CWindowsFirewallExcept()
	{
		Cleanup();
	}


	void Cleanup()
	{
		if (fwProfile != NULL)
		{
			fwProfile->Release();
			fwProfile = NULL;
		}
	}
	HRESULT Initialize()
	{
		if ( fwProfile)
			return S_FALSE;

		HRESULT hr = S_OK;
		HRESULT comInit = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
		// Ignore RPC_E_CHANGED_MODE; this just means that COM has already been
		// initialized with a different mode. Since we don't care what the mode is,
		// we'll just use the existing mode.
		if ( comInit != RPC_E_CHANGED_MODE)
		{
			hr = comInit;
			if ( FAILED(hr))
			{
				m_log.aprint("CoInitializeEx failed: 0x%08lx\n", hr);
				goto error;
			}
		}

		INetFwMgr* fwMgr = NULL;
		INetFwPolicy* fwPolicy = NULL;

		// Create an instance of the firewall settings manager.
		hr = CoCreateInstance(
			__uuidof(NetFwMgr),
			NULL,
			CLSCTX_INPROC_SERVER,
			__uuidof(INetFwMgr),
			(void**)&fwMgr
			);
		if (FAILED(hr))
		{
			m_log.aprint("CoCreateInstance failed: 0x%08lx\n", hr);
			goto error;
		}
		// Retrieve the local firewall policy.
		hr = fwMgr->get_LocalPolicy(&fwPolicy);
		if (FAILED(hr))
		{
			m_log.aprint("get_LocalPolicy failed: 0x%08lx\n", hr);
			goto error;
		}
		// Retrieve the firewall profile currently in effect.
		hr = fwPolicy->get_CurrentProfile(&fwProfile);
		if (FAILED(hr))
		{
			m_log.aprint("get_CurrentProfile failed: 0x%08lx\n", hr);
			goto error;
		}

error:
		// Release the local firewall policy.
		if (fwPolicy != NULL)
		{
			fwPolicy->Release();
		}
		// Release the firewall settings manager.
		if (fwMgr != NULL)
		{
			fwMgr->Release();
		}
		return hr;
	}

	HRESULT IsOn(OUT BOOL* fwOn)
	{
		HRESULT hr = S_OK;
		VARIANT_BOOL fwEnabled;
		ASSERT(fwProfile != NULL);
		ASSERT(fwOn != NULL);
		*fwOn = FALSE;
		// Get the current state of the firewall.
		hr = fwProfile->get_FirewallEnabled(&fwEnabled);
		if (FAILED(hr))
		{
			m_log.aprint("get_FirewallEnabled failed: 0x%08lx\n", hr);
			goto error;
		}
		// Check to see if the firewall is on.
		if (fwEnabled != VARIANT_FALSE)
		{
			*fwOn = TRUE;
			m_log.aprint("The firewall is on.\n");
		}
		else
		{
			m_log.aprint("The firewall is off.\n");
		}
error:
		return hr;
	}

	HRESULT TurnOn()
	{
		HRESULT hr = S_OK;
		BOOL fwOn;
		ASSERT(fwProfile != NULL);
		// Check to see if the firewall is off.
		hr = IsOn(&fwOn);
		if (FAILED(hr))
		{
			m_log.aprint("WindowsFirewallIsOn failed: 0x%08lx\n", hr);
			goto error;
		}
		// If it is, turn it on.
		if (!fwOn)
		{
			// Turn the firewall on.
			hr = fwProfile->put_FirewallEnabled(VARIANT_TRUE);
			if (FAILED(hr))
			{
				m_log.aprint("put_FirewallEnabled failed: 0x%08lx\n", hr);
				goto error;
			}
			m_log.aprint("The firewall is now on.\n");
		}
error:
		return hr;
	}

	HRESULT TurnOff()
	{
		HRESULT hr = S_OK;
		BOOL fwOn;
		ASSERT(fwProfile != NULL);
		// Check to see if the firewall is on.
		hr = IsOn(&fwOn);
		if (FAILED(hr))
		{
			m_log.aprint("WindowsFirewallIsOn failed: 0x%08lx\n", hr);
			goto error;
		}
		// If it is, turn it off.
		if (fwOn)
		{
			// Turn the firewall off.
			hr = fwProfile->put_FirewallEnabled(VARIANT_FALSE);
			if (FAILED(hr))
			{
				m_log.aprint("put_FirewallEnabled failed: 0x%08lx\n", hr);
				goto error;
			}
			m_log.aprint("The firewall is now off.\n");
		}
error:
		return hr;
	}

	HRESULT AppIsEnabled(
		  OUT BOOL* fwAppEnabled
		, IN const wchar_t* fwProcessImageFileName = NULL
		)
	{
		HRESULT hr = S_OK;
		BSTR fwBstrProcessImageFileName = NULL;
		VARIANT_BOOL fwEnabled;
		INetFwAuthorizedApplication* fwApp = NULL;
		INetFwAuthorizedApplications* fwApps = NULL;
		ASSERT(fwProfile != NULL);
		ASSERT(fwAppEnabled != NULL);
		*fwAppEnabled = FALSE;

		WCHAR path[MAX_PATH] = {0};
		if ( !fwProcessImageFileName)
		{
			GetModuleFileNameW(GetModuleHandle(0), path, MAX_PATH);
			fwProcessImageFileName = path;
		}

		// Retrieve the authorized application collection.
		hr = fwProfile->get_AuthorizedApplications(&fwApps);
		if (FAILED(hr))
		{
			m_log.aprint("get_AuthorizedApplications failed: 0x%08lx\n", hr);
			goto error;
		}
		// Allocate a BSTR for the process image file name.
		fwBstrProcessImageFileName = SysAllocString(fwProcessImageFileName);
		if (fwBstrProcessImageFileName == NULL)
		{
			hr = E_OUTOFMEMORY;
			m_log.aprint("SysAllocString failed: 0x%08lx\n", hr);
			goto error;
		}
		// Attempt to retrieve the authorized application.
		hr = fwApps->Item(fwBstrProcessImageFileName, &fwApp);
		if (SUCCEEDED(hr))
		{
			// Find out if the authorized application is enabled.
			hr = fwApp->get_Enabled(&fwEnabled);
			if (FAILED(hr))
			{
				m_log.aprint("get_Enabled failed: 0x%08lx\n", hr);
				goto error;
			}
			if (fwEnabled != VARIANT_FALSE)
			{
				// The authorized application is enabled.
				*fwAppEnabled = TRUE;
				m_log.aprint(
					"Authorized application %lS is enabled in the firewall.\n",
					fwProcessImageFileName
					);
			}
			else
			{
				m_log.aprint(
					"Authorized application %lS is disabled in the firewall.\n",
					fwProcessImageFileName
					);
			}
		}
		else
		{
			// The authorized application was not in the collection.
			hr = S_OK;
			m_log.aprint(
				"Authorized application %lS is disabled in the firewall.\n",
				fwProcessImageFileName
				);
		}
error:
		// Free the BSTR.
		SysFreeString(fwBstrProcessImageFileName);
		// Release the authorized application instance.
		if (fwApp != NULL)
		{
			fwApp->Release();
		}
		// Release the authorized application collection.
		if (fwApps != NULL)
		{
			fwApps->Release();
		}
		return hr;
	}

	HRESULT AddApp(
		  IN const wchar_t* fwName
		, IN const wchar_t* fwProcessImageFileName = 0)
	{
		HRESULT hr = S_OK;
		BOOL fwAppEnabled;
		BSTR fwBstrName = NULL;
		BSTR fwBstrProcessImageFileName = NULL;
		INetFwAuthorizedApplication* fwApp = NULL;
		INetFwAuthorizedApplications* fwApps = NULL;
		ASSERT(fwProfile != NULL);
		ASSERT(fwName != NULL);
		WCHAR path[MAX_PATH] = {0};
		if ( !fwProcessImageFileName)
		{
			GetModuleFileNameW(GetModuleHandle(0), path, MAX_PATH);
			fwProcessImageFileName = path;
		}

		// First check to see if the application is already authorized.
		hr = AppIsEnabled(
			&fwAppEnabled, fwProcessImageFileName);
		if (FAILED(hr))
		{
			m_log.aprint("WindowsFirewallAppIsEnabled failed: 0x%08lx\n", hr);
			goto error;
		}
		// Only add the application if it isn't already authorized.
		if (!fwAppEnabled)
		{
			// Retrieve the authorized application collection.
			hr = fwProfile->get_AuthorizedApplications(&fwApps);
			if (FAILED(hr))
			{
				m_log.aprint("get_AuthorizedApplications failed: 0x%08lx\n", hr);
				goto error;
			}
			// Create an instance of an authorized application.
			hr = CoCreateInstance(
				__uuidof(NetFwAuthorizedApplication),
				NULL,
				CLSCTX_INPROC_SERVER,
				__uuidof(INetFwAuthorizedApplication),
				(void**)&fwApp
				);
			if (FAILED(hr))
			{
				m_log.aprint("CoCreateInstance failed: 0x%08lx\n", hr);
				goto error;
			}
			// Allocate a BSTR for the process image file name.
			fwBstrProcessImageFileName = SysAllocString(fwProcessImageFileName);
			if (fwBstrProcessImageFileName == NULL)
			{
				hr = E_OUTOFMEMORY;
				m_log.aprint("SysAllocString failed: 0x%08lx\n", hr);
				goto error;
			}
			// Set the process image file name.
			hr = fwApp->put_ProcessImageFileName(fwBstrProcessImageFileName);
			if (FAILED(hr))
			{
				m_log.aprint("put_ProcessImageFileName failed: 0x%08lx\n", hr);
				goto error;
			}
			// Allocate a BSTR for the application friendly name.
			fwBstrName = SysAllocString(fwName);
			if (SysStringLen(fwBstrName) == 0)
			{
				hr = E_OUTOFMEMORY;
				m_log.aprint("SysAllocString failed: 0x%08lx\n", hr);
				goto error;
			}
			// Set the application friendly name.
			hr = fwApp->put_Name(fwBstrName);
			if (FAILED(hr))
			{
				m_log.aprint("put_Name failed: 0x%08lx\n", hr);
				goto error;
			}
			// Add the application to the collection.
			hr = fwApps->Add(fwApp);
			if (FAILED(hr))
			{
				m_log.aprint("Add failed: 0x%08lx\n", hr);
				goto error;
			}
			m_log.aprint(
				"Authorized application %lS is now enabled in the firewall.\n",
				fwProcessImageFileName
				);
		}
error:
		// Free the BSTRs.
		SysFreeString(fwBstrName);
		SysFreeString(fwBstrProcessImageFileName);
		// Release the authorized application instance.
		if (fwApp != NULL)
		{
			fwApp->Release();
		}
		// Release the authorized application collection.
		if (fwApps != NULL)
		{
			fwApps->Release();
		}
		return hr;
	}

	HRESULT PortIsEnabled(
		IN LONG portNumber,
		IN NET_FW_IP_PROTOCOL ipProtocol,
		OUT BOOL* fwPortEnabled
		)
	{
		HRESULT hr = S_OK;
		VARIANT_BOOL fwEnabled;
		INetFwOpenPort* fwOpenPort = NULL;
		INetFwOpenPorts* fwOpenPorts = NULL;
		ASSERT(fwProfile != NULL);
		ASSERT(fwPortEnabled != NULL);
		*fwPortEnabled = FALSE;

		// Retrieve the globally open ports collection.
		hr = fwProfile->get_GloballyOpenPorts(&fwOpenPorts);
		if (FAILED(hr))
		{
			m_log.aprint("get_GloballyOpenPorts failed: 0x%08lx\n", hr);
			goto error;
		}
		// Attempt to retrieve the globally open port.
		hr = fwOpenPorts->Item(portNumber, ipProtocol, &fwOpenPort);
		if (SUCCEEDED(hr))
		{
			// Find out if the globally open port is enabled.
			hr = fwOpenPort->get_Enabled(&fwEnabled);
			if (FAILED(hr))
			{
				m_log.aprint("get_Enabled failed: 0x%08lx\n", hr);
				goto error;
			}
			if (fwEnabled != VARIANT_FALSE)
			{
				// The globally open port is enabled.
				*fwPortEnabled = TRUE;
				m_log.aprint("Port %ld is open in the firewall.\n", portNumber);
			}
			else
			{
				m_log.aprint("Port %ld is not open in the firewall.\n", portNumber);
			}
		}
		else
		{
			// The globally open port was not in the collection.
			hr = S_OK;
			m_log.aprint("Port %ld is not open in the firewall.\n", portNumber);
		}
error:
		// Release the globally open port.
		if (fwOpenPort != NULL)
		{
			fwOpenPort->Release();
		}
		// Release the globally open ports collection.
		if (fwOpenPorts != NULL)
		{
			fwOpenPorts->Release();
		}
		return hr;
	}

	HRESULT PortAdd(
		IN LONG portNumber,
		IN NET_FW_IP_PROTOCOL ipProtocol,
		IN const wchar_t* name
		)
	{
		HRESULT hr = S_OK;
		BOOL fwPortEnabled = FALSE;
		BSTR fwBstrName = NULL;
		INetFwOpenPort* fwOpenPort = NULL;
		INetFwOpenPorts* fwOpenPorts = NULL;
		ASSERT(fwProfile != NULL);
		ASSERT(name != NULL);
		// First check to see if the port is already added.
		hr = PortIsEnabled(
			portNumber,
			ipProtocol,
			&fwPortEnabled
			);
		if (FAILED(hr))
		{
			m_log.aprint("WindowsFirewallPortIsEnabled failed: 0x%08lx\n", hr);
			goto error;
		}
		// Only add the port if it isn't already added.
		if (!fwPortEnabled)
		{
			// Retrieve the collection of globally open ports.
			hr = fwProfile->get_GloballyOpenPorts(&fwOpenPorts);
			if (FAILED(hr))
			{
				m_log.aprint("get_GloballyOpenPorts failed: 0x%08lx\n", hr);
				goto error;
			}
			// Create an instance of an open port.
			hr = CoCreateInstance(
				__uuidof(NetFwOpenPort),
				NULL,
				CLSCTX_INPROC_SERVER,
				__uuidof(INetFwOpenPort),
				(void**)&fwOpenPort
				);
			if (FAILED(hr))
			{
				m_log.aprint("CoCreateInstance failed: 0x%08lx\n", hr);
				goto error;
			}
			// Set the port number.
			hr = fwOpenPort->put_Port(portNumber);
			if (FAILED(hr))
			{
				m_log.aprint("put_Port failed: 0x%08lx\n", hr);
				goto error;
			}
			// Set the IP protocol.
			hr = fwOpenPort->put_Protocol(ipProtocol);
			if (FAILED(hr))
			{
				m_log.aprint("put_Protocol failed: 0x%08lx\n", hr);
				goto error;
			}
			// Allocate a BSTR for the friendly name of the port.
			fwBstrName = SysAllocString(name);
			if (SysStringLen(fwBstrName) == 0)
			{
				hr = E_OUTOFMEMORY;
				m_log.aprint("SysAllocString failed: 0x%08lx\n", hr);
				goto error;
			}
			// Set the friendly name of the port.
			hr = fwOpenPort->put_Name(fwBstrName);
			if (FAILED(hr))
			{
				m_log.aprint("put_Name failed: 0x%08lx\n", hr);
				goto error;
			}
			// Opens the port and adds it to the collection.
			hr = fwOpenPorts->Add(fwOpenPort);
			if (FAILED(hr))
			{
				m_log.aprint("Add failed: 0x%08lx\n", hr);
				goto error;
			}
			m_log.aprint("Port %ld is now open in the firewall.\n", portNumber);
		}
error:
		// Free the BSTR.
		SysFreeString(fwBstrName);
		// Release the open port instance.
		if (fwOpenPort != NULL)
		{
			fwOpenPort->Release();
		}
		// Release the globally open ports collection.
		if (fwOpenPorts != NULL)
		{
			fwOpenPorts->Release();
		}
		return hr;
	}
};

























