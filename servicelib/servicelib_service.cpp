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
// service::ControlHandler (private)
//
// Handles a service control request from the service control manager
//
// Arguments:
//
//	control			- Service control code
//	type			- Control-specific event type
//	data			- Control-specific event data

DWORD service::ControlHandler(DWORD control, DWORD type, void* data)
{
	UNREFERENCED_PARAMETER(type);
	UNREFERENCED_PARAMETER(data);

	// control should be ServiceControl type now? Keep as DWORD and static_cast<>
	// it, that way custom codes can be handled?
	if(control == SERVICE_CONTROL_INTERROGATE) return ERROR_SUCCESS;

	// TODO: only handle stop if the service implemented it
	if(control == SERVICE_CONTROL_STOP) { m_stopsignal.Set(); return ERROR_SUCCESS; }

	return NO_ERROR;
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
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	// Create a service status report function that reports to the local dispatcher
	m_statusfunc = [=](SERVICE_STATUS& status) -> void {
		// TODO
		UNREFERENCED_PARAMETER(status);
	};

	///auxiliary_state_machine::Initialize(m_name.c_str());
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
	// Define a HandlerEx callback that thunks back into this service instance
	LPHANDLER_FUNCTION_EX handler = [](DWORD control, DWORD eventtype, void* eventdata, void* context) -> DWORD { 
		return reinterpret_cast<service*>(context)->ControlHandler(control, eventtype, eventdata); };

	// Register the service control handler for this service using the handler defined above
	SERVICE_STATUS_HANDLE statushandle = RegisterServiceCtrlHandlerEx(m_name.c_str(), handler, this);
	if(statushandle == 0) throw winexception();

	// Create a service status report function that reports to the service control manager
	// using the handle provided by RegisterServiceCtrlHandlerEx above
	m_statusfunc = [=](SERVICE_STATUS& status) -> void {

		_ASSERTE(statushandle != 0);
		if(!SetServiceStatus(statushandle, &status)) throw winexception();
	};
	
	try {

		// Report a status of StartPending to the service control manager
		SetStatus(ServiceStatus::StartPending);

		// Initialize the state machine and the derived service class
		/// TODO: auxiliary_state_machine::Initialize(m_name.c_str());
		OnStart(argc, argv);
		SetStatus(ServiceStatus::Running);
		
		// Wait for the service to be stopped before continuing in the main thread
		if(WaitForSingleObject(m_stopsignal, INFINITE) == WAIT_FAILED) throw winexception();
		
		// Service is stopping; invoke all registered STOP handlers
		SetStatus(ServiceStatus::StopPending);

		//
		// Invoke all SERVICE_CONTROL_STOP handlers here, not in HandlerEx
		//

		/// TODO: auxiliary_state_machine::Terminate();

		SetStatus(ServiceStatus::Stopped);
	}

	// Set the service to STOPPED on exception.  TODO: If the service has derived
	// from a logging/tracing class that can perhaps be used here, but as far
	// as generic error reporting goes, stuck for now.
	catch(winexception& ex) { 
		
		try { SetStatus(ServiceStatus::Stopped, ex.code()); }
		catch(...) { /* CAN'T DO ANYTHING ELSE RIGHT NOW */ }
	}

	catch(...) { 
		
		try { SetStatus(ServiceStatus::Stopped, ERROR_SERVICE_SPECIFIC_ERROR, static_cast<DWORD>(E_FAIL)); }
		catch(...) { /* CAN'T DO ANYTHING ELSE RIGHT NOW */ }
	}
}

//-----------------------------------------------------------------------------
// service::SetNonPendingStatus_l (private)
//
// Sets a non-pending service status
//
// Arguments:
//
//	status				- New service status to set
//	win32exitcode		- Win32 service exit code for ServiceStatus::Stopped (see documentation)
//	serviceexitcode		- Service specific exit code for ServiceStatus::Stopped (see documentation)
//
// CAUTION: m_statuslock should be locked before calling this function
void service::SetNonPendingStatus_l(ServiceStatus status, uint32_t win32exitcode, uint32_t serviceexitcode)
{
	_ASSERTE(m_statusfunc);										// Needs to be set
	_ASSERTE(!m_statusworker.joinable());						// Should not be running

	SERVICE_STATUS newstatus;
	newstatus.dwServiceType = static_cast<DWORD>(m_processtype);
	newstatus.dwCurrentState = static_cast<DWORD>(status);
	newstatus.dwControlsAccepted = (status == ServiceStatus::Stopped) ? 0 : m_acceptedcontrols;
	newstatus.dwWin32ExitCode = (status == ServiceStatus::Stopped) ? win32exitcode : ERROR_SUCCESS;
	newstatus.dwServiceSpecificExitCode = (status == ServiceStatus::Stopped) ? serviceexitcode : ERROR_SUCCESS;
	newstatus.dwCheckPoint = 0;
	newstatus.dwWaitHint = 0;
	m_statusfunc(newstatus);
}

//-----------------------------------------------------------------------------
// service_status_manager::SetPendingStatus_l (private)
//
// Sets an automatically checkpointed pending service status
//
// Arguments:
//
//	status				- Pending service status to set
//
// CAUTION: m_statuslock should be locked before calling this function
void service::SetPendingStatus_l(ServiceStatus status)
{
	_ASSERTE(m_statusfunc);									// Needs to be set
	_ASSERTE(!m_statusworker.joinable());					// Should not be running

	// Disallow all service controls during START_PENDING and STOP_PENDING status transitions, otherwise
	// only filter out additional pending controls.  Dealing with controls received during PAUSE and CONTINUE
	// operations may or may not be valid based on the service, so that's left up to the implementation
	uint32_t accept = ((status == ServiceStatus::StartPending) || (status == ServiceStatus::StopPending)) ? 0 :
		(m_acceptedcontrols & ~(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE));

	// Kick off a new worker thread to manage the automatic checkpoint operation
	m_statusworker = std::move(std::thread([=]() {

		try {

			// Create and initialize a new SERVICE_STATUS for this operation
			SERVICE_STATUS pendingstatus;
			pendingstatus.dwServiceType = static_cast<DWORD>(m_processtype);
			pendingstatus.dwCurrentState = static_cast<DWORD>(status);
			pendingstatus.dwControlsAccepted = accept;
			pendingstatus.dwWin32ExitCode = ERROR_SUCCESS;
			pendingstatus.dwServiceSpecificExitCode = ERROR_SUCCESS;
			pendingstatus.dwCheckPoint = 1;
			pendingstatus.dwWaitHint = (status == ServiceStatus::StartPending) ? STARTUP_WAIT_HINT : PENDING_WAIT_HINT;

			// Set the initial pending status
			m_statusfunc(pendingstatus);

			// Continually report the same pending status with an incremented checkpoint until signaled
			while(WaitForSingleObject(m_statussignal, PENDING_CHECKPOINT_INTERVAL) == WAIT_TIMEOUT) {

				++pendingstatus.dwCheckPoint;
				m_statusfunc(pendingstatus);
			}
		}

		// Copy any worker thread exceptions into the m_statusexception member variable,
		// this can be checked on the next call to SetStatus()
		catch(...) { m_statusexception = std::current_exception(); }
	}));
}

//-----------------------------------------------------------------------------
// service::SetStatus (private)
//
// Sets a new service status; pending vs. non-pending is handled automatically
//
// Arguments:
//
//	status			- New service status to set
//	win32exitcode	- Win32 specific exit code for ServiceStatus::Stopped (see documentation)
//	serviceexitcode	- Service-specific exit code for ServiceStatus::Stopped (see documentation)

void service::SetStatus(ServiceStatus status, uint32_t win32exitcode, uint32_t serviceexitcode)
{
	lock critsec(m_statuslock);					// Automatic CRITICAL_SECTION wrapper

	// Check for a duplicate status; pending states are managed automatically
	if(status == m_status) return;

	// Check for a pending service state operation
	if(m_statusworker.joinable()) {

		// Cancel the pending state checkpoint thread by signaling the event object
		if(SignalObjectAndWait(m_statussignal, m_statusworker.native_handle(), INFINITE, FALSE) == WAIT_FAILED) 
			throw winexception();

		m_statusworker.join();
		m_statussignal.Reset();

		// Check for the presence of an exception from the worker thread and rethrow it
		if(m_statusexception) 
			std::rethrow_exception(m_statusexception);
	}

	// Invoke the proper status helper based on the type of status being set
	switch(status) {

		// Pending status codes
		case ServiceStatus::StartPending:
		case ServiceStatus::StopPending:
		case ServiceStatus::ContinuePending:
		case ServiceStatus::PausePending:
			SetPendingStatus_l(status);
			break;

		// Non-pending status codes without an exit status
		case ServiceStatus::Running:
		case ServiceStatus::Paused:
			SetNonPendingStatus_l(status);
			break;

		// Non-pending status codes that report exit status
		case ServiceStatus::Stopped:
			SetNonPendingStatus_l(status, win32exitcode, serviceexitcode);
			break;

		// Invalid status code
		default: throw winexception(E_INVALIDARG);
	}
	
	m_status = status;					// Service status has been changed
}

/////////// TODO: Keep this?
void service::Stop(DWORD win32exitcode, DWORD serviceexitcode)
{
	UNREFERENCED_PARAMETER(win32exitcode);
	UNREFERENCED_PARAMETER(serviceexitcode);

	m_stopsignal.Set();
	// Save codes here for final status call
}

END_NAMESPACE(svctl)

//-----------------------------------------------------------------------------

#pragma pop_macro("END_NAMESPACE")
#pragma pop_macro("BEGIN_NAMESPACE")
#pragma warning(pop)
