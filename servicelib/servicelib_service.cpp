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
// service Constructor
//
// Arguments:
//
//	name			- Service name
//	processtype		- Servuice process type
//	handlers		- Service control handler table

service::service(const tchar_t* name, ServiceProcessType processtype, const control_handler_table& handlers) 
	: m_name(name), m_processtype(processtype), m_handlers(handlers), m_acceptedcontrols([&]() -> DWORD {

	DWORD accept = 0;

	// Derive what controls this service should report based on what service control handlers
	// have been implemented in the derived class
	for(const auto& iterator : handlers) {

		if(iterator->Control == ServiceControl::Stop)						accept |= SERVICE_ACCEPT_STOP;
		else if(iterator->Control == ServiceControl::Pause)					accept |= SERVICE_ACCEPT_PAUSE_CONTINUE;
		else if(iterator->Control == ServiceControl::Continue)				accept |= SERVICE_ACCEPT_PAUSE_CONTINUE;
		else if(iterator->Control == ServiceControl::Shutdown)				accept |= SERVICE_ACCEPT_SHUTDOWN;
		else if(iterator->Control == ServiceControl::ParameterChange)		accept |= SERVICE_ACCEPT_PARAMCHANGE;
		else if(iterator->Control == ServiceControl::NetBindAdd)			accept |= SERVICE_ACCEPT_NETBINDCHANGE;
		else if(iterator->Control == ServiceControl::NetBindRemove)			accept |= SERVICE_ACCEPT_NETBINDCHANGE;
		else if(iterator->Control == ServiceControl::NetBindEnable)			accept |= SERVICE_ACCEPT_NETBINDCHANGE;
		else if(iterator->Control == ServiceControl::NetBindDisable)		accept |= SERVICE_ACCEPT_NETBINDCHANGE;
		else if(iterator->Control == ServiceControl::HardwareProfileChange)	accept |= SERVICE_ACCEPT_HARDWAREPROFILECHANGE;
		else if(iterator->Control == ServiceControl::PowerEvent)			accept |= SERVICE_ACCEPT_POWEREVENT;
		else if(iterator->Control == ServiceControl::SessionChange)			accept |= SERVICE_ACCEPT_SESSIONCHANGE;
		else if(iterator->Control == ServiceControl::PreShutdown)			accept |= SERVICE_ACCEPT_PRESHUTDOWN;
		else if(iterator->Control == ServiceControl::TimeChange)			accept |= SERVICE_ACCEPT_TIMECHANGE;
		else if(iterator->Control == ServiceControl::TriggerEvent)			accept |= SERVICE_ACCEPT_TRIGGEREVENT;
		else if(iterator->Control == ServiceControl::UserModeReboot)		accept |= SERVICE_ACCEPT_USERMODEREBOOT;
	}

	return accept;						// Return the generated bitmask

}()) {}

//-----------------------------------------------------------------------------
// service::ControlHandler (private)
//
// Handles a service control request from the service control manager
//
// Arguments:
//
//	control			- Service control code
//	eventtype		- Control-specific event type
//	eventdata		- Control-specific event data

DWORD service::ControlHandler(ServiceControl control, DWORD eventtype, void* eventdata)
{
	// Get the current status of the service (note: This is technically a race
	// condition with SetStatus(), but should be fine here; use m_statuslock if not)
	ServiceStatus currentstatus = m_status;

	// Nothing should be coming in from the service control manager when stopped
	if(currentstatus == ServiceStatus::Stopped) return ERROR_CALL_NOT_IMPLEMENTED;

	// PAUSE controls are allowed to be redundant by the service control manager
	// and should not be accepted if the service is not in a RUNNING state
	if(control == ServiceControl::Pause) {

		if((currentstatus == ServiceStatus::Paused) || (currentstatus == ServiceStatus::PausePending)) return ERROR_SUCCESS;
		if(currentstatus != ServiceStatus::Running) return ERROR_CALL_NOT_IMPLEMENTED;
	}

	// CONTINUE controls are allowed to be redundant by the service control manager
	// and should not be accepted if the service is not in a PAUSED state
	else if(control == ServiceControl::Continue) {

		if((currentstatus == ServiceStatus::Running) || (currentstatus == ServiceStatus::ContinuePending)) return ERROR_SUCCESS;
		if(currentstatus != ServiceStatus::Paused) return ERROR_CALL_NOT_IMPLEMENTED;
	}

	// INTERROGATE, STOP, PAUSE and CONTINUE are all special case handlers
	if(control == ServiceControl::Interrogate) return ERROR_SUCCESS;
	else if(control == ServiceControl::Stop) { m_stopsignal.Set(); return ERROR_SUCCESS; }
	else if(control == ServiceControl::Pause) { m_pausesignal.Set(); return ERROR_SUCCESS; }
	else if(control == ServiceControl::Continue) { m_continuesignal.Set(); return ERROR_SUCCESS; }

	// Iterate over all of the implemented control handlers and invoke each of them
	// in the order in which they appear in the control handler vector<>
	bool handled = false;
	for(const auto& iterator : m_handlers) {

		if(iterator->Control != control) continue;

		// Invoke the service control handler; if a non-zero result is returned stop
		// processing them and return that result back to the service control manager
		DWORD result = iterator->Invoke(this, eventtype, eventdata);
		if(result != ERROR_SUCCESS) return result;
		
		handled = true;						// At least one handler was successfully invoked
	}

	// Default for most service controls is to return ERROR_SUCCESS if it was handled
	// and ERROR_CALL_NOT_IMPLEMENTED if no handler was present for the control
	return (handled) ? ERROR_SUCCESS : ERROR_CALL_NOT_IMPLEMENTED;
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
		return reinterpret_cast<service*>(context)->ControlHandler(static_cast<ServiceControl>(control), eventtype, eventdata); };

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

		// Initialize registered auxiliary base classes
		/// TODO: auxiliary_state_machine::Initialize(m_name.c_str());

		// Invoke the derived service's OnStart, when that returns service is running
		OnStart(argc, argv);
		SetStatus(ServiceStatus::Running);

		// primary_handler lambda; Used to set a pending status, invoke all relevant handler functions for the
		// signaled control, set the proper non-pending status and reset the event object when finished
		std::function<void(ServiceStatus, ServiceControl, ServiceStatus, const signal<signal_type::ManualReset>&)> primary_handler = 
			[this](ServiceStatus pending, ServiceControl control, ServiceStatus final, const signal<signal_type::ManualReset>& signal) -> void {

			SetStatus(pending);						// Set the specified pending status

			// Iterate over all the derived class handlers and invoke each relevant one in the order they appear
			for(const auto& handler : m_handlers) { if(handler->Control == control) handler->Invoke(this, 0, nullptr); }

			SetStatus(final);						// Set final status
			signal.Reset();							// Reset the kernel event object to unsignaled
		};

		// The primary service control functions (STOP, PAUSE and CONTINUE) are handled here in the
		// main service thread rather than in the control handler, and are signaled via event objects
		std::array<HANDLE, 3> signals = { m_stopsignal, m_pausesignal, m_continuesignal };
		DWORD wait = WAIT_FAILED;

		// Only loop until the STOP event has been signaled
		while(wait != WAIT_OBJECT_0) {

			// Wait for one of the three primary service control events to be signaled
			wait = WaitForMultipleObjects(signals.size(), signals.data(), FALSE, INFINITE);

			// WAIT_OBJECT_0 --> ServiceControl::Stop
			if(wait == WAIT_OBJECT_0)
				primary_handler(ServiceStatus::StopPending, ServiceControl::Stop, ServiceStatus::Stopped, m_stopsignal);
			
			// WAIT_OBJECT_0 + 1 --> ServiceControl::Pause
			else if(wait == WAIT_OBJECT_0 + 1)
				primary_handler(ServiceStatus::PausePending, ServiceControl::Pause, ServiceStatus::Paused, m_pausesignal);

			// WAIT_OBJECT_0 + 2 --> ServiceControl::Continue
			else if(wait == WAIT_OBJECT_0 + 2)
				primary_handler(ServiceStatus::ContinuePending, ServiceControl::Continue, ServiceStatus::Running, m_continuesignal);

			// WAIT_FAILED --> Error
			else if(wait == WAIT_FAILED) throw winexception();
			else throw winexception(E_FAIL);
		}
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

	// Create and initialize a new SERVICE_STATUS for this operation
	SERVICE_STATUS newstatus;
	newstatus.dwServiceType = static_cast<DWORD>(m_processtype);
	newstatus.dwCurrentState = static_cast<DWORD>(status);
	newstatus.dwControlsAccepted = (status == ServiceStatus::Stopped) ? 0 : m_acceptedcontrols;
	newstatus.dwWin32ExitCode = (status == ServiceStatus::Stopped) ? win32exitcode : ERROR_SUCCESS;
	newstatus.dwServiceSpecificExitCode = (status == ServiceStatus::Stopped) ? serviceexitcode : ERROR_SUCCESS;
	newstatus.dwCheckPoint = 0;
	newstatus.dwWaitHint = 0;

	m_statusfunc(newstatus);					// Set the non-pending status
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

	// Block all controls during SERVICE_START_PENDING and SERVICE_STOP_PENDING only
	DWORD accept = ((status == ServiceStatus::StartPending) || (status == ServiceStatus::StopPending)) ? 
		0 : m_acceptedcontrols;

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

END_NAMESPACE(svctl)

//-----------------------------------------------------------------------------

#pragma pop_macro("END_NAMESPACE")
#pragma pop_macro("BEGIN_NAMESPACE")
#pragma warning(pop)
