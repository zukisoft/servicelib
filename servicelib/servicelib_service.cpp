//-----------------------------------------------------------------------------
// Copyright (c) 2001-2014 Michael G. Brehm
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "servicelib.h"

#pragma warning(push, 4)

// BEGIN_NAMESPACE / END_NAMESPACE
//
#pragma push_macro("BEGIN_NAMESPACE")
#pragma push_macro("END_NAMESPACE")
#undef BEGIN_NAMESPACE
#undef END_NAMESPACE
#define BEGIN_NAMESPACE(ns)		namespace ns {
#define END_NAMESPACE(ns)		}

//-----------------------------------------------------------------------------

BEGIN_NAMESPACE(svctl)

//-----------------------------------------------------------------------------
// service
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// service::ControlRequest (private)
//
// Initiates a service control request
//
// Arguments:
//
//	control			- Service control code
//	type			- Control-specific event type
//	data			- Control-specific event data

uint32_t service::ControlRequest(uint32_t control, uint32_t type, void* data)
{
	return NO_ERROR;
}

//-----------------------------------------------------------------------------
// service::ControlRequestCallback (private, static)
//
// Control handler callback registered with the service control manager
//
// Arguments:
//
//	control			- Service control code
//	type			- Control-specific event type
//	data			- Control-specific event data
//	context			- Context pointer passed into RegisterServiceCtrlHandlerEx()

DWORD WINAPI service::ControlRequestCallback(DWORD control, DWORD type, void* data, void* context)
{
	_ASSERTE(context != nullptr);
	return static_cast<DWORD>(reinterpret_cast<service*>(context)->ControlRequest(control, type, data));
}

//-----------------------------------------------------------------------------
// service::LocalMain
//
// Service entry point when process is being run as a normal application
//
// Arguments:
//
//	argc		- Number of command line arguments
//	argv		- Array of command-line argument strings

void service::LocalMain(uint32_t argc, tchar_t** argv)
{
	auxiliary_state_machine::Initialize(GetServiceName());
}

//-----------------------------------------------------------------------------
// service::ServiceMain
//
// Service entry point when process is being run as a service
//
// Arguments:
//
//	argc		- Number of command line arguments
//	argv		- Array of command-line argument strings

void service::ServiceMain(uint32_t argc, tchar_t** argv)
{
	// Register the service control handler for this service
	SERVICE_STATUS_HANDLE statushandle = RegisterServiceCtrlHandlerEx(GetServiceName(), ControlRequestCallback, this);
	if(statushandle == 0) throw winexception();

	// Create a service status manager for this service that will report changes
	// directly to the service control manager
	m_statusmgr = std::make_unique<service_status_manager>(GetServiceType(), statushandle, 0, 
		[](SERVICE_STATUS_HANDLE handle, SERVICE_STATUS status) -> void {
		
		// Report the status to the service control manager
		if(!SetServiceStatus(handle, &status)) throw winexception();
	});

	// Report StartPending as quickly as possible while we get things ready
	m_statusmgr->Set(ServiceStatus::StartPending);

	auxiliary_state_machine::Initialize(GetServiceName());

	// OnStart(argc, argv)
	// SERVICE_RUNNING

	// Run
	// SERVICE_STOP_PENDING

	// state_machine::Terminate
	// SERVICE_STOPPED

	//////////////
	// Once RegisterServiceCtrlHandlerEx has been called successfully, we can
	// report error codes via the service control manager status
	//////////////
}

END_NAMESPACE(svctl)

//-----------------------------------------------------------------------------

#pragma pop_macro("END_NAMESPACE")
#pragma pop_macro("BEGIN_NAMESPACE")
#pragma warning(pop)
