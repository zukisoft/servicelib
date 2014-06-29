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
#include "ServiceHarness.h"

#pragma warning(push, 4)

namespace svctl {

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
	zero_init(m_status).dwCurrentState = static_cast<DWORD>(ServiceStatus::Stopped);
}

//-----------------------------------------------------------------------------
// service_harness Destructor

service_harness::~service_harness()
{
	// If the main service thread is still active, it needs to be detached
	// (There doesn't appear to be a legitimate way to also kill it)
	if(m_mainthread.joinable()) m_mainthread.detach();
}

//-----------------------------------------------------------------------------
// service_harness::AppendToMultiStringBuffer (private)
//
// Appends a single string to an existing REG_MULTI_SZ buffer or terminates the
// buffer by appending a final NULL if nullptr has been specified for string
//
// Arguments:
//
//	buffer		- Parameter byte data buffer
//	string		- String to append to the buffer, or nullptr to terminate

std::vector<uint8_t>& service_harness::AppendToMultiStringBuffer(std::vector<uint8_t>& buffer, const tchar_t* string)
{
	// Calculate the length of the data to be appended to the buffer and get current position
	size_t length = (string) ? ((_tcslen(string) + 1) * sizeof(tchar_t)) : sizeof(tchar_t);
	size_t pos = buffer.size();

	// Allocate sufficient space in the buffer to hold the new data
	buffer.resize(buffer.size() + length);

	// If a string was provided, copy that and the NUL terminator into the buffer.  Otherwise,
	// append a single NULL by zeroing out the additional space reserved above
	if(string) memcpy_s(buffer.data() + pos, buffer.size() - pos, string, length);
	else memset(buffer.data() + pos, 0, length);

	return buffer;
}

//-----------------------------------------------------------------------------
// service_harness::CloseParameterStoreFunc (private)
//
// Function invoked by the service to close parameter storage
//
// Arguments:
//
//	handle		- Handle provided by OpenParameterStore

void service_harness::CloseParameterStoreFunc(void* handle)
{
	_ASSERTE(handle == reinterpret_cast<void*>(this));
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
// service_harness::LoadParameterFunc (private)
//
// Function invoked by the service to load a parameter value
//
// Arguments:
//
//	handle		- Handle provided by OpenParameterStore
//	name		- Name of the parameter to load
//	format		- Expected parameter data format
//	buffer		- Destination buffer for the parameter data
//	length		- Length of the parameter destination buffer

size_t service_harness::LoadParameterFunc(void* handle, const tchar_t* name, ServiceParameterFormat format, void* buffer, size_t length)
{
	// The handle provided by OpenParameterStore is fake; it's just the (this) pointer
	_ASSERTE(handle == reinterpret_cast<void*>(this));
	if(handle != reinterpret_cast<void*>(this)) return ERROR_INVALID_PARAMETER;

	std::lock_guard<std::recursive_mutex> critsec(m_paramlock);

	// If a buffer has been provided, initialize it to all zeros
	if(buffer) memset(buffer, 0, length);

	// Locate the parameter in the collection --> ERROR_FILE_NOT_FOUND if it doesn't exist
	auto iterator = m_parameters.find(tstring(name));
	if(iterator == m_parameters.end()) throw winexception(ERROR_FILE_NOT_FOUND);

	// Check the data type against the stored value --> ERROR_UNSUPPORTED_TYPE if doesn't match
	if(iterator->second.first != format) throw winexception(ERROR_UNSUPPORTED_TYPE);

	if(buffer) {
			
		// check the buffer length and copy the data --> ERROR_MORE_DATA if insufficient
		if(length < iterator->second.second.size()) throw winexception(ERROR_MORE_DATA);
		else memcpy_s(buffer, length, iterator->second.second.data(), iterator->second.second.size());
	}

	return iterator->second.second.size();			// Return the size of the parameter value in bytes
}

//-----------------------------------------------------------------------------
// service_harness::OpenParameterStoreFunc (private)
//
// Function invoked by the contained service to open parameter storage (no-op)
//
// Arguments:
//
//	servicename		- Name of the service instance

void* service_harness::OpenParameterStoreFunc(const tchar_t* servicename)
{
	UNREFERENCED_PARAMETER(servicename);

	// A handle isn't necessary for the local parameter store, use (this)
	return reinterpret_cast<void*>(this); 
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
// service_harness::RegisterHandlerFunc (private)
//
// Function invoked by the service to register it's control handler
//
// Arguments:
//
//	servicename		- Name assigned to the service instance
//	handler			- Pointer to the service's HandlerEx() callback
//	context			- HandlerEx() callback context pointer

SERVICE_STATUS_HANDLE service_harness::RegisterHandlerFunc(LPCTSTR servicename, LPHANDLER_FUNCTION_EX handler, LPVOID context)
{
	_ASSERTE(handler != nullptr);
	UNREFERENCED_PARAMETER(servicename);

	m_handler = handler;					// Store the handler function pointer
	m_context = context;					// Store the handler context pointer

	// Return the address of this harness instance as a pseudo SERVICE_STATUS_HANDLE for the service
	return reinterpret_cast<SERVICE_STATUS_HANDLE>(this);
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
// service_harness::SetParameter
//
// Sets a string-based parameter key/value pair
//
// Arguments:
//
//	name		- Name of the parameter to set
//	value		- Value to assign to the parameter

void service_harness::SetParameter(const resstring& name, const tchar_t* value)
{
	// If a non-null string pointer was provided, set it, otherwise set a blank string instead
	if(value) SetParameter(name, ServiceParameterFormat::String, value, (_tcslen(value) + 1) * sizeof(tchar_t));
	else SetParameter(name, ServiceParameterFormat::String, _T("\0"), sizeof(tchar_t));
}

//-----------------------------------------------------------------------------
// service_harness::SetParameter
//
// Sets a string-based parameter key/value pair
//
// Arguments:
//
//	name		- Name of the parameter to set
//	value		- Value to assign to the parameter

void service_harness::SetParameter(const resstring& name, const tstring& value)
{
	// Set the parameter using the const tchar_t pointer for the string, include the null terminator
	SetParameter(name, ServiceParameterFormat::String, value.c_str(), (value.length() + 1) * sizeof(tchar_t));
}

//-----------------------------------------------------------------------------
// service_harness::SetParameter (private)
//
// Internal implementation of SetParameter, accepts the type and raw data
//
// Arguments:
//
//	name		- Parameter name
//	format		- Parameter format
//	value		- Pointer to the parameter data
//	length		- Length, in bytes, of the parameter data

void service_harness::SetParameter(const tstring& name, ServiceParameterFormat format, const void* value, size_t length)
{
	_ASSERTE(value);
	_ASSERTE(length > 0);

	// Use pointers to the data as the iterators for constructing the vector<>
	const uint8_t* begin = reinterpret_cast<const uint8_t*>(value);
	const uint8_t* end = begin + length;

	// Insert or replace the parameter value in the collection
	SetParameter(name, format, std::vector<uint8_t>(begin, end));
}

//-----------------------------------------------------------------------------
// service_harness::SetParameter (private)
//
// Internal implementation of SetParameter, accepts the type and raw data
//
// Arguments:
//
//	name		- Parameter name
//	format		- Parameter format
//	value		- Parameter data as a vector<> rvalue reference

void service_harness::SetParameter(const tstring& name, ServiceParameterFormat format, std::vector<uint8_t>&& value)
{
	if(name.length() == 0) throw winexception(ERROR_INVALID_PARAMETER);

	std::lock_guard<std::recursive_mutex> critsec(m_paramlock);
	m_parameters[name] = parameter_value(format, std::move(value));
}

//-----------------------------------------------------------------------------
// service_harness::SetStatusFunc (private)
//
// Function invoked by the service to report a change in status
//
// Arguments:
//
//	handle		- Handle returned by RegisterHandlerFunc
//	status		- New status being reported by the service

BOOL service_harness::SetStatusFunc(SERVICE_STATUS_HANDLE handle, LPSERVICE_STATUS status)
{
	std::unique_lock<std::mutex> critsec(m_statuslock);

	// Ensure that the handle provided is actually the address of this harness instance
	_ASSERTE(reinterpret_cast<service_harness*>(handle) == this);
	if(reinterpret_cast<service_harness*>(handle) != this) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }

	m_status = *status;						// Copy the new SERVICE_STATUS
	critsec.unlock();						// Release the critical section
	m_statuschanged.notify_all();			// Notify the status has been changed

	return TRUE;
};

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
	using namespace std::placeholders;

	// If the main thread has already been created, the service has already been started
	if(m_mainthread.joinable()) throw winexception(ERROR_SERVICE_ALREADY_RUNNING);

	// Always reset the SERVICE_STATUS back to defaults before starting the service
	zero_init(m_status).dwCurrentState = static_cast<DWORD>(ServiceStatus::Stopped);

	// There is an expectation that argv[0] is set to the service name
	if((argvector.size() == 0) || (argvector[0].length() == 0)) throw winexception(E_INVALIDARG);

	// Create the main service thread and launch it
	m_mainthread = std::move(std::thread([=]() {

		// Create a copy of the arguments vector local to this thread so it won't be destroyed
		// prematurely, and convert it into an argv-style array of generic text string pointers
		std::vector<tstring> arguments(argvector);
		std::vector<tchar_t*> argv;
		for(const auto& arg: arguments) argv.push_back(const_cast<tchar_t*>(arg.c_str()));
		argv.push_back(nullptr);

		// Create the context for the service by binding this instance's member functions
		service_context context = { 
			ServiceProcessType::Unique,
			std::bind(&service_harness::RegisterHandlerFunc, this, _1, _2, _3),
			std::bind(&service_harness::SetStatusFunc, this, _1, _2),
			std::bind(&service_harness::OpenParameterStoreFunc, this, _1),
			std::bind(&service_harness::LoadParameterFunc, this, _1, _2, _3, _4, _5),
			std::bind(&service_harness::CloseParameterStoreFunc, this, _1) 
		};

		// Launch the service with the specified command line arguments and instance context
		LaunchService(static_cast<int>(argv.size() - 1), argv.data(), context);
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

};	// namespace svctl

//-----------------------------------------------------------------------------

