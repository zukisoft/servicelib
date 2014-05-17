#include "stdafx.h"
#include "service_params.h"

#pragma warning(push, 4)

namespace svctl {

//-----------------------------------------------------------------------------
// svctl::
//-----------------------------------------------------------------------------

void service_parameters::OnStart(const tchar_t* name, ServiceProcessType, int, tchar_t**)
{
	_ASSERTE(name);

	// Construct the path to the service parameter registry key
	tstring	keypath = tstring(_T("System\\CurrentControlSet\\Services\\")) + name + _T("\\Parameters");

	// Attempt to open/create the registry key for the service parameters
	if(RegCreateKeyEx(HKEY_LOCAL_MACHINE, keypath.c_str(), 0, nullptr, 0,
		KEY_READ | KEY_WRITE, nullptr, &m_keyhandle, nullptr) != ERROR_SUCCESS) throw winexception();

	//for(auto iterator : m_
}

};	// namespace svctl

//-----------------------------------------------------------------------------
// ::ServiceParameters
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

