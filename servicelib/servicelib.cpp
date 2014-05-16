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

namespace svctl {

//-----------------------------------------------------------------------------
// svctl::auxiliary_state_machine
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// auxiliary_state_machine::RegisterAuxiliaryState (protected)
//
// Registers an auxiliary class with the state machine.  This should be called
// from the constructor of an auxiliary class that derives from auxiliary_state_machine
// as a virtual private base class
//
// Arguments:
//
//	instance		- auxiliary_state interface instance pointer

void auxiliary_state_machine::RegisterAuxiliaryState(struct auxiliary_state* instance)
{
	_ASSERTE(instance != nullptr);
	if(!instance) throw winexception(E_INVALIDARG);

	m_instances.push_back(instance);
}

//-----------------------------------------------------------------------------
// svctl::resstring
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// resstring::GetResourceString (private, static)
//
// Gets a read-only pointer to a module string resource
//
// Arguments:
//
//	id			- Resource identifier code
//	instance	- Module instance handle to acquire the resource from

const tchar_t* resstring::GetResourceString(unsigned int id, HINSTANCE instance)
{
	static const tchar_t EMPTY[] = _T("");		// Used for missing resources

	// LoadString() has a neat trick to return a read-only string pointer
	tchar_t* string = nullptr;
	int result = LoadString(instance, id, reinterpret_cast<tchar_t*>(&string), 0);

	// Return an empty string rather than a null pointer if the resource was missing
	return (result == 0) ? EMPTY : string;
}

//-----------------------------------------------------------------------------
// svctl::service
//-----------------------------------------------------------------------------

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
	lock critsec(m_statuslock);

	// Nothing should be coming in from the service control manager when stopped
	if(m_status == ServiceStatus::Stopped) return ERROR_CALL_NOT_IMPLEMENTED;

	///// TODO: these aren't quite right based on the model I chose; the way I did
	// this the CONTINUE event should queue up after PAUSE not be declined
	//
	// PAUSE controls are allowed to be redundant by the service control manager
	// and should not be accepted if the service is not in a RUNNING state
	if(control == ServiceControl::Pause) {

		if((m_status == ServiceStatus::Paused) || (m_status == ServiceStatus::PausePending)) return ERROR_SUCCESS;
		if(m_status != ServiceStatus::Running) return ERROR_CALL_NOT_IMPLEMENTED;
	}

	// CONTINUE controls are allowed to be redundant by the service control manager
	// and should not be accepted if the service is not in a PAUSED state
	else if(control == ServiceControl::Continue) {

		if((m_status == ServiceStatus::Running) || (m_status == ServiceStatus::ContinuePending)) return ERROR_SUCCESS;
		if(m_status != ServiceStatus::Paused) return ERROR_CALL_NOT_IMPLEMENTED;
	}

	// INTERROGATE, STOP, PAUSE and CONTINUE are all special case handlers
	if(control == ServiceControl::Interrogate) return ERROR_SUCCESS;
	else if(control == ServiceControl::Stop) { 
		
		if(!m_statusworker.joinable()) SetPendingStatus(ServiceStatus::StopPending);
		m_stopsignal.Set(); 
		return ERROR_SUCCESS; 
	}
	else if(control == ServiceControl::Pause) { 
		
		if(!m_statusworker.joinable()) SetPendingStatus(ServiceStatus::PausePending);
		m_pausesignal.Set(); 
		return ERROR_SUCCESS; 
	}
	else if(control == ServiceControl::Continue) { 
		
		if(!m_statusworker.joinable()) SetPendingStatus(ServiceStatus::ContinuePending);
		m_continuesignal.Set(); 
		return ERROR_SUCCESS; 
	}

	// Done with messing about with the current service status; release the critsec
	critsec.Release();

	// Iterate over all of the implemented control handlers and invoke each of them
	// in the order in which they appear in the control handler vector<>
	bool handled = false;
	for(const auto& iterator : Handlers) {

		if(iterator->Control != control) continue;

		// Invoke the service control handler; if a non-zero result is returned stop
		// processing them and return that result back to the service control manager
		DWORD result = iterator->Invoke(this, eventtype, eventdata);
		if(result != ERROR_SUCCESS) return result;
		
		handled = true;				// At least one handler was successfully invoked
	}

	// Default for most service controls is to return ERROR_SUCCESS if it was handled
	// and ERROR_CALL_NOT_IMPLEMENTED if no handler was present for the control
	return (handled) ? ERROR_SUCCESS : ERROR_CALL_NOT_IMPLEMENTED;
}

//-----------------------------------------------------------------------------
// service::getAcceptedControls (private)
//
// Determines what SERVICE_ACCEPT_XXXX codes the service will respond to based
// on what control handlers have been registered
//
// Arguments:
//
//	NONE

DWORD service::getAcceptedControls(void) const
{
	DWORD accept = 0;

	// Derive what controls this service should report based on what service control handlers have
	// been implemented in the derived class
	for(const auto& iterator : Handlers) {

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
}

//-----------------------------------------------------------------------------
// service::getHandlers (protected, virtual)
//
// Gets the collection of service-specific control handlers

const control_handler_table& service::getHandlers(void) const
{
	// The default implementation has no handlers; this results in a service that
	// can be started but otherwise will not respond to any controls, including STOP
	static control_handler_table nohandlers;
	return nohandlers;
}

//-----------------------------------------------------------------------------
// service::Run (protected)
//
// Service instance entry point
//
// Arguments:
//
//	name			- Service name
//	processtype		- Service process type
//	argc			- Number of command line arguments
//	argv			- Array of command line argument strings

void service::Run(const tchar_t* name, ServiceProcessType processtype, int argc, tchar_t** argv)
{
	_ASSERTE(name);										// Should never be a NULL pointer on entry

	// Define a static HandlerEx callback that calls back into this service instance
	LPHANDLER_FUNCTION_EX handler = [](DWORD control, DWORD eventtype, void* eventdata, void* context) -> DWORD { 
		return reinterpret_cast<service*>(context)->ControlHandler(static_cast<ServiceControl>(control), eventtype, eventdata); };

	// Register a service control handler for this service instance
	SERVICE_STATUS_HANDLE statushandle = RegisterServiceCtrlHandlerEx(name, handler, this);
	if(statushandle == 0) throw winexception();

	// Define a status reporting function that uses the handle and process types defined above
	m_statusfunc = [=](SERVICE_STATUS& status) -> void {

		_ASSERTE(statushandle != 0);
		status.dwServiceType = static_cast<DWORD>(processtype);
		if(!SetServiceStatus(statushandle, &status)) throw winexception();
	};

	try {

		// START --> StartPending --> aux::OnStart() --> OnStart() --> Running
		SetStatus(ServiceStatus::StartPending);
		auxiliary_state_machine::OnStart(argc, argv);
		OnStart(argc, argv);
		SetStatus(ServiceStatus::Running);

		// The primary service control functions (STOP, PAUSE and CONTINUE) are handled here in the
		// main service thread rather than in the control handler, and are signaled via event objects
		std::array<HANDLE, 3> signals = { m_stopsignal, m_pausesignal, m_continuesignal };
		DWORD wait = WAIT_FAILED;

		// Only loop until the STOP event has been signaled
		while(wait != WAIT_OBJECT_0) {

			// Wait for one of the three primary service control events to be signaled
			wait = WaitForMultipleObjects(static_cast<DWORD>(signals.size()), signals.data(), FALSE, INFINITE);

			// WAIT_OBJECT_0: STOP --> StopPending --> OnStop() --> aux::OnStop() --> Stopped
			if(wait == WAIT_OBJECT_0) {

				SetStatus(ServiceStatus::StopPending);
				for(const auto& handler : Handlers) { if(handler->Control == ServiceControl::Stop) handler->Invoke(this, 0, nullptr); }
				auxiliary_state_machine::OnStop();
				SetStatus(ServiceStatus::Stopped);
				m_stopsignal.Reset();
			}
			
			// WAIT_OBJECT_0 + 1: PAUSE --> PausePending --> OnPause() --> aux::OnPause() --> Paused
			else if(wait == WAIT_OBJECT_0 + 1) {

				SetStatus(ServiceStatus::PausePending);
				for(const auto& handler : Handlers) { if(handler->Control == ServiceControl::Pause) handler->Invoke(this, 0, nullptr); }
				auxiliary_state_machine::OnPause();
				SetStatus(ServiceStatus::Paused);
				m_pausesignal.Reset();
			}

			// WAIT_OBJECT_0 + 2: CONTINUE --> ContinuePending --> aux::OnContinue() --> OnContinue() --> Running
			else if(wait == WAIT_OBJECT_0 + 2) {
				
				SetStatus(ServiceStatus::ContinuePending);
				auxiliary_state_machine::OnContinue();
				for(const auto& handler : Handlers) { if(handler->Control == ServiceControl::Continue) handler->Invoke(this, 0, nullptr); }
				SetStatus(ServiceStatus::Running);
				m_continuesignal.Reset();
			}

			// WAIT_FAILED: Error
			else if(wait == WAIT_FAILED) throw winexception();
			else throw winexception(E_FAIL);
		}
	}

	catch(winexception& ex) { 
		
		// Set the service to STOPPED on an unhandled winexception
		try { SetStatus(ServiceStatus::Stopped, ex.code()); }
		catch(...) { /* CAN'T DO ANYTHING ELSE RIGHT NOW */ }
	}

	catch(...) { 
		
		// Set the service to STOPPED on an unhandled exception
		try { SetStatus(ServiceStatus::Stopped, ERROR_SERVICE_SPECIFIC_ERROR, static_cast<DWORD>(E_FAIL)); }
		catch(...) { /* CAN'T DO ANYTHING ELSE RIGHT NOW */ }
	}
}

//-----------------------------------------------------------------------------
// service::SetNonPendingStatus (private)
//
// Sets a non-pending service status
//
// Arguments:
//
//	status				- New service status to set
//	win32exitcode		- Win32 service exit code for ServiceStatus::Stopped (see documentation)
//	serviceexitcode		- Service specific exit code for ServiceStatus::Stopped (see documentation)

void service::SetNonPendingStatus(ServiceStatus status, uint32_t win32exitcode, uint32_t serviceexitcode)
{
	lock critsec(m_statuslock);

	_ASSERTE(m_statusfunc);							// Needs to be set
	_ASSERTE(!m_statusworker.joinable());			// Should not be running

	// Create and initialize a new SERVICE_STATUS for this operation
	SERVICE_STATUS newstatus;
	newstatus.dwServiceType = 0;	// <-- set in the report_status_func
	newstatus.dwCurrentState = static_cast<DWORD>(status);
	newstatus.dwControlsAccepted = (status == ServiceStatus::Stopped) ? 0 : AcceptedControls;
	newstatus.dwWin32ExitCode = (status == ServiceStatus::Stopped) ? win32exitcode : ERROR_SUCCESS;
	newstatus.dwServiceSpecificExitCode = (status == ServiceStatus::Stopped) ? serviceexitcode : ERROR_SUCCESS;
	newstatus.dwCheckPoint = 0;
	newstatus.dwWaitHint = 0;

	m_statusfunc(newstatus);						// Set the non-pending status
}

//-----------------------------------------------------------------------------
// service::SetPendingStatus (private)
//
// Sets an automatically checkpointed pending service status
//
// Arguments:
//
//	status				- Pending service status to set

void service::SetPendingStatus(ServiceStatus status)
{
	lock critsec(m_statuslock);

	_ASSERTE(m_statusfunc);							// Needs to be set
	_ASSERTE(!m_statusworker.joinable());			// Should not be running

	// Block all controls during SERVICE_START_PENDING and SERVICE_STOP_PENDING only
	DWORD accept = ((status == ServiceStatus::StartPending) || (status == ServiceStatus::StopPending)) ? 
		0 : AcceptedControls;

	// Kick off a new worker thread to manage the automatic checkpoint operation
	m_statusworker = std::move(std::thread([=]() {

		try {

			// Create and initialize a new SERVICE_STATUS for this operation
			SERVICE_STATUS pendingstatus;
			pendingstatus.dwServiceType = 0;	// <-- set in the report_status_func
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
	lock critsec(m_statuslock);				// Automatic CRITICAL_SECTION wrapper

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
			SetPendingStatus(status);
			break;

		// Non-pending status codes without an exit status
		case ServiceStatus::Running:
		case ServiceStatus::Paused:
			SetNonPendingStatus(status);
			break;

		// Non-pending status codes that report exit status
		case ServiceStatus::Stopped:
			SetNonPendingStatus(status, win32exitcode, serviceexitcode);
			break;

		// Invalid status code
		default: throw winexception(E_INVALIDARG);
	}
	
	m_status = status;					// Service status has been changed
}

//-----------------------------------------------------------------------------
// svctl::winexception
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// winexception Constructor
//
// Arguments:
//
//	result		- Win32 error code

winexception::winexception(DWORD result) : m_code(result)
{
	char_t*				formatted;				// Formatted message

	// Invoke FormatMessageA to convert the system error code into an ANSI string; use a lame
	// generic 'unknown' string for any codes that cannot be looked up successfully
	if(FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, result,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<char_t*>(&formatted), 0, nullptr)) {
		
		m_what = formatted;						// Store the formatted message string
		LocalFree(formatted);					// Release FormatMessage() allocated buffer
	}

	else m_what = "Unknown Windows status code " + std::to_string(result);
}

};	// namespace svctl


//-----------------------------------------------------------------------------
// ::ServiceModule
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// ::ServiceTable
//-----------------------------------------------------------------------------

// InvokeServiceStub (ServiceTable friend)
//
// Helper funtion used to access private s_stubs variable in ServiceTable
inline void InvokeServiceStub(size_t index, DWORD argc, LPTSTR* argv)
{
	_ASSERTE(index < ServiceTable::MAX_SERVICES);
	ServiceTable::s_stubs[index].Invoke(argc, argv);
}

// ServiceTable::s_stubs
//
// In order to support dynamically named services, a collection of predefined 
// ServiceMain entry points still need to exist.  Each service_stub contains such
// a unique entry point that calls back into itself to get the name, process type
// and service class entry point set by the Dispatch() method
std::array<svctl::service_stub, ServiceTable::MAX_SERVICES> ServiceTable::s_stubs = {

	// This is ugly, but the lambda must be declared with the LPSERVICE_MAIN_FUNCTION protoype, which
	// prevents capturing any arguments, therefore the indexes must be hard-coded for now

	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(0, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(1, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(2, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(3, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(4, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(5, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(6, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(7, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(8, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(9, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(10, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(11, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(12, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(13, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(14, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(15, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(16, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(17, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(18, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(19, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(20, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(21, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(22, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(23, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(24, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(25, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(26, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(27, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(28, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(29, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(30, argc, argv); }),
	svctl::service_stub([](DWORD argc, LPTSTR* argv) -> void { InvokeServiceStub(31, argc, argv); }),
};

//-----------------------------------------------------------------------------
// ServiceTable::Dispatch
//
// Dispatches the collection of services to the service control manager
//
// Arguments:
//
//	services		- Collection of service_entry objects

int ServiceTable::Dispatch(void)
{
	// Can only have as many services as there are service_stubs defined
	if(vector::size() > s_stubs.size()) return E_BOUNDS;

	// Determine the service process type to set based on the number of services
	ServiceProcessType processtype = (vector::size() > 1) ? ServiceProcessType::Shared : ServiceProcessType::Unique;

	// Convert the collection into SERVICE_TABLE_ENTRY structures for the service control manager
	std::vector<SERVICE_TABLE_ENTRY> table;
	for(size_t index = 0; index < vector::size(); index++) {

		// The static stub requires the correct name, process type and real entry point set
		const svctl::service_table_entry& entry = vector::at(index);
		s_stubs[index].Set(entry.Name, processtype, entry.ServiceMain);

		table.push_back(s_stubs[index]);				// Insert into the collection
	}
	table.push_back( { nullptr, nullptr } );			// Table needs to end with NULLs

	// Attempt to start the service control dispatcher
	if(!StartServiceCtrlDispatcher(table.data())) return static_cast<int>(GetLastError());

	return 0;
}

//-----------------------------------------------------------------------------

