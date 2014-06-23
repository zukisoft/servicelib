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
// svctl::GetServiceProcessType
//
// Reads the service process type flags from the registry
//
// Arguments:
//
//	name		- Service key name

ServiceProcessType GetServiceProcessType(const tchar_t* name)
{
	HKEY			key;						// Service registry key
	DWORD			value = 0;					// REG_DWORD value buffer
	DWORD			cb = sizeof(value);			// Size of value buffer

	// Attempt to open the services registry key with read-only access
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services"), 0, KEY_READ, &key) == ERROR_SUCCESS) {
		
		// Attempt to grab the Type REG_DWORD value from the registry and close the key
		RegGetValue(key, name, _T("Type"), RRF_RT_REG_DWORD, nullptr, &value, &cb);
		RegCloseKey(key);
	}

	return static_cast<ServiceProcessType>(value);
}

//-----------------------------------------------------------------------------
// svctl::parameter_base
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// parameter_base::Bind
//
// Binds the parameter to the parent registry key
//
// Arguments:
//
//	key		- Parent Parameters registry key handle
//	name	- Registry value name to bind the parameter to

void parameter_base::Bind(HKEY key, const tchar_t* name) 
{
	std::lock_guard<std::recursive_mutex> critsec(m_lock);

	m_key = key;
	m_name = name;
}

//-----------------------------------------------------------------------------
// parameter_base::IsBound (protected)
//
// Determines if the parameter has been bound to the registry or not
//
// Arguments:
//
//	NONE

bool parameter_base::IsBound(void)
{
	std::lock_guard<std::recursive_mutex> critsec(m_lock);
	return (m_key != nullptr);
}

//-----------------------------------------------------------------------------
// parameter_base::Unbind
//
// Unbinds the parameter from the parent registry key
//
// Arguments:
//
//	NONE

void parameter_base::Unbind(void)
{
	std::lock_guard<std::recursive_mutex> critsec(m_lock);

	m_key = nullptr;
	m_name.clear();
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
// service::Continue
//
// Continues the service from a paused state
//
// Arguments:
//
//	NONE

DWORD service::Continue(void)
{
	std::lock_guard<std::recursive_mutex> critsec(m_statuslock);

	// Service has to be in a status of PAUSED to accept this control
	if(m_status != ServiceStatus::Paused) return ERROR_CALL_NOT_IMPLEMENTED;
	SetStatus(ServiceStatus::ContinuePending);

	// Use a single pooled thread to invoke all of the CONTINUE handlers
	std::async(std::launch::async, [=]() {

		for(const auto& handler : Handlers) if(handler->Control == ServiceControl::Continue) handler->Invoke(this, 0, nullptr);
		SetStatus(ServiceStatus::Running);
	});

	return ERROR_SUCCESS;
}

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
	std::unique_lock<std::recursive_mutex> critsec(m_statuslock);

	// Nothing should be coming in from the service control manager when stopped
	if(m_status == ServiceStatus::Stopped) return ERROR_CALL_NOT_IMPLEMENTED;

	// INTERROGATE, STOP, PAUSE and CONTINUE are special case handlers
	if(control == ServiceControl::Interrogate) return ERROR_SUCCESS;
	else if(control == ServiceControl::Stop) { Stop(); return ERROR_SUCCESS; }
	else if(control == ServiceControl::Pause) { Pause(); return ERROR_SUCCESS; }
	else if(control == ServiceControl::Continue) { Continue(); return ERROR_SUCCESS; }

	// Done with messing about with the current service status; release the critsec
	critsec.unlock();

	// PARAMCHANGE is automatically accepted if there are any parameters in the service,
	// but the derived class may also want to handle it; call it here but don't return
	if(control == ServiceControl::ParameterChange) ReloadParameters();

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
// service::IterateParameters (protected)
//
// Iterates over all parameters defined for the service and executes a function
//
// Arguments:
//
//	func		- Function to pass each parameter name and reference to

void service::IterateParameters(std::function<void(const tstring& name, parameter_base& param)> func)
{
	// Default implementation has no parameters to iterate
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

DWORD service::getAcceptedControls(void)
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

	// If there are any svctl parameters in the service class, auto-accept PARAMCHANGE
	IterateParameters([&](const tstring&, parameter_base&) { accept |= SERVICE_ACCEPT_PARAMCHANGE; });

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
// service::Pause
//
// Pauses the service
//
// Arguments:
//
//	NONE

DWORD service::Pause(void)
{
	std::lock_guard<std::recursive_mutex> critsec(m_statuslock);

	// Service has to be in a status of RUNNING to accept this control
	if(m_status != ServiceStatus::Running) return ERROR_CALL_NOT_IMPLEMENTED;
	SetStatus(ServiceStatus::PausePending);

	// Use a single pooled thread to invoke all of the PAUSE handlers
	std::async(std::launch::async, [=]() {

		for(const auto& handler : Handlers) if(handler->Control == ServiceControl::Pause) handler->Invoke(this, 0, nullptr);
		SetStatus(ServiceStatus::Paused);
	});

	return ERROR_SUCCESS;
}

//-----------------------------------------------------------------------------
// service::ReloadParameters
//
// Reloads the values for all bound parameter member variables
//
// Arguments:
//
//	NONE

void service::ReloadParameters(void)
{
	// This can be done asynchronously; just iterate each parameter and reload it's value from the registry
	std::async(std::launch::async, [=]() { IterateParameters([=](const tstring&, parameter_base& param) { param.Load(); }); });
}

//-----------------------------------------------------------------------------
// service::Main
//
// Service instance entry point
//
// Arguments:
//
//	argc				- Number of command line arguments
//	argv				- Array of command line argument strings
//	context				- Service runtime context information and callbacks

void service::Main(int argc, tchar_t** argv, const service_context& context)
{
	HKEY keyparams = nullptr;						// Service parameters registry key

	_ASSERTE(context.RegisterHandlerFunc);
	_ASSERTE(context.SetStatusFunc);

	// Define a static HandlerEx callback that calls back into this service instance
	LPHANDLER_FUNCTION_EX handler = [](DWORD control, DWORD eventtype, void* eventdata, void* context) -> DWORD { 
		return reinterpret_cast<service*>(context)->ControlHandler(static_cast<ServiceControl>(control), eventtype, eventdata); };

	// Register a service control handler for this service instance
	SERVICE_STATUS_HANDLE statushandle = context.RegisterHandlerFunc(argv[0], handler, this);
	if(statushandle == 0) throw winexception();

	// Define a status reporting function that uses the handle and process type defined above
	m_statusfunc = [=](SERVICE_STATUS& status) -> void {

		_ASSERTE(statushandle != 0);
		status.dwServiceType = static_cast<DWORD>(context.ProcessType);
		if(!context.SetStatusFunc(statushandle, &status)) throw winexception();
	};

	try {

		// Service is starting; report SERVICE_START_PENDING
		SetStatus(ServiceStatus::StartPending);

		// Open or create the Parameters registry key for this service and bind the parameters
		//
		// TODO: This doesn't work for local applications not running as administrator, deal with that
		// perhaps by abstracting the entire method for reading parameters (command line? xml?).  Once
		// that is fixed up, put back the exception result
		//
		//if(RegCreateKeyEx(HKEY_LOCAL_MACHINE, (tstring(_T("System\\CurrentControlSet\\Services\\")) + argv[0] + _T("\\Parameters")).c_str(), 
		//	0, nullptr, 0, KEY_READ | KEY_WRITE, nullptr, &keyparams, nullptr) != ERROR_SUCCESS) throw winexception();
		//IterateParameters([=](const tstring& name, parameter_base& param) { param.Bind(keyparams, name.c_str()); param.Load(); });

		// for now just bind/load only if RegCreateKeyEx() is actually successful
		if(RegCreateKeyEx(HKEY_LOCAL_MACHINE, (tstring(_T("System\\CurrentControlSet\\Services\\")) + argv[0] + _T("\\Parameters")).c_str(), 
			0, nullptr, 0, KEY_READ | KEY_WRITE, nullptr, &keyparams, nullptr) == ERROR_SUCCESS) {

			IterateParameters([=](const tstring& name, parameter_base& param) { param.Bind(keyparams, name.c_str()); param.Load(); });
		}

		// Invoke derived service class startup code
		OnStart(argc, argv);

		// Service is now running, wait for the event indicating SERVICE_STOPPED has been set
		SetStatus(ServiceStatus::Running);
		WaitForSingleObject(m_stopsignal, INFINITE);
	}

	catch(winexception& ex) { 
		
		// Set the service to STOPPED on an unhandled winexception, translate ERROR_SUCCESS into ERROR_SERVICE_SPECIFIC
		try { SetStatus(ServiceStatus::Stopped, (ex.code() != ERROR_SUCCESS) ? ex.code() : ERROR_SERVICE_SPECIFIC_ERROR); }
		catch(...) { /* CAN'T DO ANYTHING ELSE RIGHT NOW */ }
	}

	catch(...) { 
		
		// Set the service to STOPPED on an unhandled exception
		try { SetStatus(ServiceStatus::Stopped, ERROR_SERVICE_SPECIFIC_ERROR, static_cast<DWORD>(E_FAIL)); }
		catch(...) { /* CAN'T DO ANYTHING ELSE RIGHT NOW */ }
	}

	// Unbind all of the service parameters and close the parameters registry key
	//
	// TODO: moved these here on purpose, but should this be done before setting SERVICE_STOPPED?
	IterateParameters([](const tstring&, parameter_base& param) { param.Unbind(); });
	if(keyparams) RegCloseKey(keyparams);
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
	std::lock_guard<std::recursive_mutex> critsec(m_statuslock);

	_ASSERTE(m_statusfunc);							// Needs to be set
	_ASSERTE(!m_statusworker.joinable());			// Should not be running

	// Create and initialize a new SERVICE_STATUS for this operation
	SERVICE_STATUS newstatus;
	newstatus.dwServiceType = 0;		// <-- Set by m_statusfunc
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
	std::lock_guard<std::recursive_mutex> critsec(m_statuslock);

	_ASSERTE(m_statusfunc);							// Needs to be set
	_ASSERTE(!m_statusworker.joinable());			// Should not be running

	// Block all controls during SERVICE_START_PENDING and SERVICE_STOP_PENDING, otherwise only block
	// controls that would result in a service status change while a status change is pending
	DWORD accept = ((status == ServiceStatus::StartPending) || (status == ServiceStatus::StopPending)) ? 0 
		: (AcceptedControls & ~(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE | SERVICE_ACCEPT_SHUTDOWN));

	// Set the initial pending status before launching the worker thread
	SERVICE_STATUS newstatus;
	newstatus.dwServiceType = 0;			// <-- Set by m_statusfunc
	newstatus.dwCurrentState = static_cast<DWORD>(status);
	newstatus.dwControlsAccepted = accept;
	newstatus.dwWin32ExitCode = ERROR_SUCCESS;
	newstatus.dwServiceSpecificExitCode = ERROR_SUCCESS;
	newstatus.dwCheckPoint = 1;
	newstatus.dwWaitHint = (status == ServiceStatus::StartPending) ? STARTUP_WAIT_HINT : PENDING_WAIT_HINT;
	m_statusfunc(newstatus);

	// Kick off a new worker thread to manage the automatic checkpoint operation
	m_statusworker = std::move(std::thread([=]() {

		try {

			// Copy the previous SERVICE_STATUS into a structure on the thread's stack
			SERVICE_STATUS pendingstatus = newstatus;

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
	std::lock_guard<std::recursive_mutex> critsec(m_statuslock);

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
		if(m_statusexception) std::rethrow_exception(m_statusexception);
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
	
	m_status = status;						// Service status has been changed		
}

//-----------------------------------------------------------------------------
// service::Stop
//
// Stops the service
//
// Arguments:
//
//	win32exitcode		- Win32 service exit code
//	serviceexitcode		- Service specific exit code

DWORD service::Stop(DWORD win32exitcode, DWORD serviceexitcode)
{
	std::lock_guard<std::recursive_mutex> critsec(m_statuslock);

	// Service cannot be stopped unless it's RUNNING or PAUSED, this could cause
	// potential race conditions in the derived service class; better to block it
	if(m_status != ServiceStatus::Running && m_status != ServiceStatus::Paused) return ERROR_CALL_NOT_IMPLEMENTED;
	SetStatus(ServiceStatus::StopPending);

	// Use a single pooled thread to invoke all of the STOP handlers
	std::async(std::launch::async, [=]() {

		for(const auto& handler : Handlers) if(handler->Control == ServiceControl::Stop) handler->Invoke(this, 0, nullptr);
		SetStatus(ServiceStatus::Stopped, win32exitcode, serviceexitcode);

		m_stopsignal.Set();				// Signal the exit from the main thread
	});

	return ERROR_SUCCESS;
}

//-----------------------------------------------------------------------------
// svctl::service_harness
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// service_harness Constructor
//
// Arguments:
//
//	NONE

service_harness::service_harness()
{
	// Initialize the SERVICE_STATUS to the default state
	memset(&m_status, 0, sizeof(SERVICE_STATUS));
	m_status.dwCurrentState = static_cast<DWORD>(ServiceStatus::Stopped);
}

//-----------------------------------------------------------------------------
// service_harness Destructor

service_harness::~service_harness()
{
	// If the main service thread is still active, it needs to be detached
	// and terminated to avoid a runtime error when the process exits
	if(m_mainthread.joinable()) {

		HANDLE thread = m_mainthread.native_handle();
		m_mainthread.detach();
		TerminateThread(thread, static_cast<DWORD>(E_ABORT));
	}
}

//-----------------------------------------------------------------------------
// service_harness::Continue
//
// Sends SERVICE_CONTROL_CONTINUE to the service, throws an exception if the
// service does not accept the control or stops prematurely
//
// Arguments:
//
//	NONE

void service_harness::Continue(void)
{
	DWORD result = SendControl(ServiceControl::Continue);
	if(result != ERROR_SUCCESS) throw winexception(result);

	WaitForStatus(ServiceStatus::Running);
}

//-----------------------------------------------------------------------------
// service_harness::getCanContinue
//
// Determines if the service can be continued

bool service_harness::getCanContinue(void)
{
	std::lock_guard<std::mutex> critsec(m_statuslock);

	// If the service is not running, it cannot be controlled at all
	if(!m_mainthread.joinable()) return false;

	// The service can be continued if it's in a PAUSED state and accepts the control
	return ((static_cast<ServiceStatus>(m_status.dwCurrentState) == ServiceStatus::Paused) && 
		ServiceControlAccepted(ServiceControl::Continue, m_status.dwControlsAccepted));
}

//-----------------------------------------------------------------------------
// service_harness::getCanPause
//
// Determines if the service can be paused

bool service_harness::getCanPause(void)
{
	std::lock_guard<std::mutex> critsec(m_statuslock);

	// If the service is not running, it cannot be controlled at all
	if(!m_mainthread.joinable()) return false;

	// The service can be paused if it's in a RUNNING state and accepts the control
	return ((static_cast<ServiceStatus>(m_status.dwCurrentState) == ServiceStatus::Running) && 
		ServiceControlAccepted(ServiceControl::Pause, m_status.dwControlsAccepted));
}

//-----------------------------------------------------------------------------
// service_harness::getCanStop
//
// Determines if the service can be stopped

bool service_harness::getCanStop(void)
{
	std::lock_guard<std::mutex> critsec(m_statuslock);

	// If the service is not running, it cannot be controlled at all
	if(!m_mainthread.joinable()) return false;

	// The service can be stopped if it's not in a STOPPED or STOP_PENDING state and accepts the control
	return ((static_cast<ServiceStatus>(m_status.dwCurrentState) != ServiceStatus::Stopped) && 
		(static_cast<ServiceStatus>(m_status.dwCurrentState) != ServiceStatus::StopPending) &&
		ServiceControlAccepted(ServiceControl::Stop, m_status.dwControlsAccepted));
}

//-----------------------------------------------------------------------------
// service_harness::Pause
//
// Sends SERVICE_CONTROL_PAUSE to the service, throws an exception if the
// service does not accept the control or stops prematurely
//
// Arguments:
//
//	NONE

void service_harness::Pause(void)
{
	DWORD result = SendControl(ServiceControl::Pause);
	if(result != ERROR_SUCCESS) throw winexception(result);

	WaitForStatus(ServiceStatus::Paused);
}

//-----------------------------------------------------------------------------
// service_harness::SendControl
//
// Sends a control to the service
//
// Arguments:
//
//	control		- Control to be sent to the service
//	eventtype	- Specifies a control-specific event type code (uncommon)
//	eventdata	- Specifies control-specific event data (uncommon)

DWORD service_harness::SendControl(ServiceControl control, DWORD eventtype, void* eventdata)
{
	std::unique_lock<std::mutex> critsec(m_statuslock);

	// If the main service thread is not running, that's the same result as SERVICE_STOPPED
	if(!m_mainthread.joinable()) return ERROR_SERVICE_NOT_ACTIVE;

	// Certain service statuses prevent the service from being controlled at all, see ControlService() on MSDN
	switch(static_cast<ServiceStatus>(m_status.dwCurrentState)) {

		// SERVICE_STOPPED and SERVICE_STOP_PENDING never allow for a new control to be sent
		case ServiceStatus::Stopped : return ERROR_SERVICE_NOT_ACTIVE;
		case ServiceStatus::StopPending: return ERROR_SERVICE_CANNOT_ACCEPT_CTRL;

		// SERVICE_START_PENDING only allows SERVICE_CONTROL_STOP
		case ServiceStatus::StartPending : 
			if(control != ServiceControl::Stop) return ERROR_SERVICE_CANNOT_ACCEPT_CTRL;
			// fall-through

		// Check the requested control against the service's current accepted controls mask
		default: if(!ServiceControlAccepted(control, m_status.dwControlsAccepted)) return ERROR_INVALID_SERVICE_CONTROL;
	}

	// Unlock the status critical section and invoke the service's handler directly
	critsec.unlock();
	return m_handler(static_cast<DWORD>(control), eventtype, eventdata, m_context);
}

//-----------------------------------------------------------------------------
// service_harness::ServiceControlAccepted (private, static)
//
// Checks a ServiceControl against a SERVICE_ACCEPT_XXXX mask
//
// Arguments:
//
//	control			- Service control code to be checked
//	mask			- SERVICE_ACCEPT_XXXX mask to check control against

bool service_harness::ServiceControlAccepted(ServiceControl control, DWORD mask)
{
	switch(control) {
		
		case ServiceControl::Stop:					return ((mask & SERVICE_ACCEPT_STOP) == SERVICE_ACCEPT_STOP);
		case ServiceControl::Pause:					return ((mask & SERVICE_ACCEPT_PAUSE_CONTINUE) == SERVICE_ACCEPT_PAUSE_CONTINUE);
		case ServiceControl::Continue:				return ((mask & SERVICE_ACCEPT_PAUSE_CONTINUE) == SERVICE_ACCEPT_PAUSE_CONTINUE);
		case ServiceControl::Interrogate:			return true;
		case ServiceControl::Shutdown:				return ((mask & SERVICE_ACCEPT_SHUTDOWN) == SERVICE_ACCEPT_SHUTDOWN);
		case ServiceControl::ParameterChange:		return ((mask & SERVICE_ACCEPT_PARAMCHANGE) == SERVICE_ACCEPT_PARAMCHANGE);
		case ServiceControl::NetBindAdd:			return ((mask & SERVICE_ACCEPT_NETBINDCHANGE) == SERVICE_ACCEPT_NETBINDCHANGE);
		case ServiceControl::NetBindRemove:			return ((mask & SERVICE_ACCEPT_NETBINDCHANGE) == SERVICE_ACCEPT_NETBINDCHANGE);
		case ServiceControl::NetBindEnable:			return ((mask & SERVICE_ACCEPT_NETBINDCHANGE) == SERVICE_ACCEPT_NETBINDCHANGE);
		case ServiceControl::NetBindDisable:		return ((mask & SERVICE_ACCEPT_NETBINDCHANGE) == SERVICE_ACCEPT_NETBINDCHANGE);
		case ServiceControl::DeviceEvent:			return false;
		case ServiceControl::HardwareProfileChange:	return ((mask & SERVICE_ACCEPT_HARDWAREPROFILECHANGE) == SERVICE_ACCEPT_HARDWAREPROFILECHANGE);
		case ServiceControl::PowerEvent:			return ((mask & SERVICE_ACCEPT_POWEREVENT) == SERVICE_ACCEPT_POWEREVENT);
		case ServiceControl::SessionChange:			return ((mask & SERVICE_ACCEPT_SESSIONCHANGE) == SERVICE_ACCEPT_SESSIONCHANGE);
		case ServiceControl::PreShutdown:			return ((mask & SERVICE_ACCEPT_PRESHUTDOWN) == SERVICE_ACCEPT_PRESHUTDOWN);
		case ServiceControl::TimeChange:			return ((mask & SERVICE_ACCEPT_TIMECHANGE) == SERVICE_ACCEPT_TIMECHANGE);
		case ServiceControl::TriggerEvent:			return ((mask & SERVICE_ACCEPT_TRIGGEREVENT) == SERVICE_ACCEPT_TRIGGEREVENT);
		case ServiceControl::UserModeReboot:		return ((mask & SERVICE_ACCEPT_USERMODEREBOOT) == SERVICE_ACCEPT_USERMODEREBOOT);

		default: return false;
	}
}

//-----------------------------------------------------------------------------
// service_harness::Start (private)
//
// Starts the service
//
// Arguments:
//
//	argvector		- vector<> of command line argument strings

void service_harness::Start(std::vector<tstring>& argvector)
{
	// If the main thread has already been created, the service has already been started
	if(m_mainthread.joinable()) throw winexception(ERROR_SERVICE_ALREADY_RUNNING);

	// Always reset the SERVICE_STATUS back to defaults before starting the service
	memset(&m_status, 0, sizeof(SERVICE_STATUS));
	m_status.dwCurrentState = static_cast<DWORD>(ServiceStatus::Stopped);

	// There is an expectation that argv[0] is set to the service name
	if((argvector.size() == 0) || (argvector[0].length() == 0)) throw winexception(E_INVALIDARG);

	// Define the control handler registration callback for this harness instance
	register_handler_func registerfunc = ([=](LPCTSTR servicename, LPHANDLER_FUNCTION_EX handler, LPVOID context) -> SERVICE_STATUS_HANDLE 
	{
		_ASSERTE(handler != nullptr);
		UNREFERENCED_PARAMETER(servicename);

		m_handler = handler;					// Store the handler function pointer
		m_context = context;					// Store the handler context pointer

		// Return the address of this harness instance as a pseudo SERVICE_STATUS_HANDLE for the service
		return reinterpret_cast<SERVICE_STATUS_HANDLE>(this);
	});

	// Define the status change callback for this harness instance
	set_status_func statusfunc = ([=](SERVICE_STATUS_HANDLE handle, LPSERVICE_STATUS status) -> BOOL { 

		std::unique_lock<std::mutex> critsec(m_statuslock);

		// Ensure that the handle provided is actually the address of this harness instance
		_ASSERTE(reinterpret_cast<service_harness*>(handle) == this);
		if(reinterpret_cast<service_harness*>(handle) != this) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }

		m_status = *status;						// Copy the new SERVICE_STATUS
		critsec.unlock();						// Release the critical section
		m_statuschanged.notify_all();			// Notify the status has been changed

		return TRUE;
	});

	// Create the main service thread and launch it
	m_mainthread = std::move(std::thread([=]() {

		// Create a copy of the arguments vector local to this thread so it won't be destroyed
		// prematurely, and convert it into an argv-style array of generic text string pointers
		std::vector<tstring> arguments(argvector);
		std::vector<tchar_t*> argv;
		for(const auto& arg: arguments) argv.push_back(const_cast<tchar_t*>(arg.c_str()));
		argv.push_back(nullptr);

		// Create the context for the service using the lambdas defined above and invoke LocalMain()
		service_context context = { ServiceProcessType::Unique, registerfunc, statusfunc };
		LaunchService(argv.size() - 1, argv.data(), context);
	}));

	// Wait up to 30 seconds for the service to set SERVICE_START_PENDING
	if(!WaitForStatus(ServiceStatus::StartPending, 30000)) throw winexception(ERROR_SERVICE_REQUEST_TIMEOUT);

	// Wait indefinitely for the service to set SERVICE_RUNNING
	WaitForStatus(ServiceStatus::Running);
}

//-----------------------------------------------------------------------------
// service_harness::Stop
//
// Sends SERVICE_CONTROL_STOP to the service, throws an exception if the
// service does not accept the control or stops prematurely
//
// Arguments:
//
//	NONE

void service_harness::Stop(void)
{
	DWORD result = SendControl(ServiceControl::Stop);
	if(result != ERROR_SUCCESS) throw winexception(result);

	WaitForStatus(ServiceStatus::Stopped);
}

//-----------------------------------------------------------------------------
// service_harness::WaitForStatus
//
// Waits for the service to reach the specified status
//
// Arguments:
//
//	status		- Service status to wait for
//	timeout		- Amount of time, in milliseconds, to wait before failing

bool service_harness::WaitForStatus(ServiceStatus status, uint32_t timeout)
{
	std::unique_lock<std::mutex> critsec(m_statuslock);

	// Wait for the condition variable to be trigged with the service status caller is looking for, or if
	// the service has stopped unexpectedly due to an unhandled exception caught in ServiceMain()
	bool result = m_statuschanged.wait_until(critsec, std::chrono::system_clock::now() + std::chrono::milliseconds(timeout), [=]() 
	{ 
		return (static_cast<ServiceStatus>(m_status.dwCurrentState) == status) || 
			((static_cast<ServiceStatus>(m_status.dwCurrentState) == ServiceStatus::Stopped) && (m_status.dwWin32ExitCode != ERROR_SUCCESS)); 
	});

	// If the service has stopped (regardless of the reason), wait for the main thread to terminate
	if(static_cast<ServiceStatus>(m_status.dwCurrentState) == ServiceStatus::Stopped) m_mainthread.join();

	// If an error was generated by the service, throw that as an exception to the caller
	if(m_status.dwWin32ExitCode != ERROR_SUCCESS) throw winexception(m_status.dwWin32ExitCode);

	return result;
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
// ::ServiceTable
//-----------------------------------------------------------------------------

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
	// Convert the collection into a table of SERVICE_TABLE_ENTRY structures
	std::vector<SERVICE_TABLE_ENTRY> table;
	for(size_t index = 0; index < vector::size(); index++) {

		const svctl::service_table_entry& entry = vector::at(index);
		table.push_back( { const_cast<LPTSTR>(entry.Name), entry.ServiceMain } );
	}

	table.push_back( { nullptr, nullptr } );		// Table needs to end with NULLs

	// Attempt to start the service control dispatcher
	if(!StartServiceCtrlDispatcher(table.data())) return static_cast<int>(GetLastError());

	return 0;
}

//-----------------------------------------------------------------------------

