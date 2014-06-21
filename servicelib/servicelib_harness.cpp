#include "stdafx.h"
#include "servicelib.h"
#pragma warning(push, 4)

namespace svctl {


	
service_harness::service_harness()
{
	// Initialize the SERVICE_STATUS structure to the proper defaults
	memset(&m_status, 0, sizeof(SERVICE_STATUS));
	m_status.dwServiceType = static_cast<DWORD>(ServiceProcessType::Unique);
	m_status.dwCurrentState = static_cast<DWORD>(ServiceStatus::Stopped);
}

//-----------------------------------------------------------------------------
// service_harness::Continue
//
// Sends SERVICE_CONTROL_CONTINUE to the service, throws an exception if the
// service does not accept the control
//
// Arguments:
//
//	NONE

void service_harness::Continue(void)
{
	DWORD result = SendControl(ServiceControl::Continue);
	if(result != ERROR_SUCCESS) throw winexception(result);
}

// This isn't something a normal client can do ... but useful in the harness
DWORD service_harness::SendControl(ServiceControl control, DWORD eventtype, void* eventdata)
{
	(control);
	(eventdata);
	(eventtype);

	return 0;
}

//-----------------------------------------------------------------------------
// service_harness::Pause
//
// Sends SERVICE_CONTROL_PAUSE to the service, throws an exception if the
// service does not accept the control
//
// Arguments:
//
//	NONE

void service_harness::Pause(void)
{
	DWORD result = SendControl(ServiceControl::Pause);
	if(result != ERROR_SUCCESS) throw winexception(result);
}

//DWORD service_harness::Stop(void)
//{
	// watch for ERROR_SERVICE_REQUEST_TIMEOUT - invoke handler asynchronously
	// with a signal and wait on that for 30 seconds

	// SERVICE_STOPPED --> ERROR_SERVICE_NOT_ACTIVE (always)
	// SERVICE_STOP_PENDING --> ERROR_SERVICE_CANNOT_ACCEPT_CTRL  (always)

	// SERVICE_START_PENDING --> anything but STOP --> ERROR_SERVICE_CANNOT_ACCEPT_CTRL
		
	// else check mask


	//if(!m_mainthread.joinable()) return ERROR_SERVICE_NOT_ACTIVE;

	//if(m_status.dwCurrentState == SERVICE_STOPPED) return ERROR_SERVICE_CANNOT_ACCEPT_CTRL;
	//if((m_status.dwCurrentState == SERVICE_START_PENDING) || (m_status.dwCurrentState == SERVICE_STOP_PENDING)) return ERROR_SERVICE_CANNOT_ACCEPT_CTRL;

	//// Check to see if the service accepts the STOP control
	//if((m_status.dwControlsAccepted & SERVICE_ACCEPT_STOP) != SERVICE_ACCEPT_STOP) return ERROR_INVALID_SERVICE_CONTROL;

	//return m_handler(static_cast<DWORD>(ServiceControl::Stop), 0, nullptr, m_context);
//}

void service_harness::WaitForStop(void)
{
	// this really isn't quite right, but works for now
	m_mainthread.join();
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

	// Wait up to 30 seconds for the service to set SERVICE_START_PENDING before giving up
	if(!WaitForStatus(ServiceStatus::StartPending, 30000)) throw winexception(ERROR_SERVICE_REQUEST_TIMEOUT);
}

//-----------------------------------------------------------------------------
// service_harness::Stop
//
// Sends SERVICE_CONTROL_STOP to the service, throws an exception if the
// service does not accept the control
//
// Arguments:
//
//	NONE

void service_harness::Stop(void)
{
	DWORD result = SendControl(ServiceControl::Stop);
	if(result != ERROR_SUCCESS) throw winexception(result);
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

	// Wait for the condition variable to be trigged with the service status caller is looking for
	return m_statuschanged.wait_until(critsec, std::chrono::system_clock::now() + std::chrono::milliseconds(timeout),
		[=]() { return static_cast<ServiceStatus>(m_status.dwCurrentState) == status; });
}

//////////////////////

};

#pragma warning(pop)

