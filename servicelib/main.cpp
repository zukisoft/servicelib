// servicelib.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "servicelib.h"
#include "resource.h"
#include <mutex>
#include <condition_variable>


// MinimalService Sample
//
class MinimalService : public Service<MinimalService>
{
public:

	// Constructor / Destructor
	MinimalService()=default;
	virtual ~MinimalService()=default;

private:

	MinimalService(const MinimalService&)=delete;
	MinimalService& operator=(const MinimalService&)=delete;

	// CONTROL_HANDLER_MAP
	//
	// Series of ATL-like macros that define a mapping of service
	// control codes to handler functions
	BEGIN_CONTROL_HANDLER_MAP(MinimalService)
		CONTROL_HANDLER_ENTRY(ServiceControl::Stop, OnStop)
	END_CONTROL_HANDLER_MAP()

	// OnStart (Service)
	//
	// Every service that derives from Service<> must define an OnStart()
	void OnStart(int argc, LPTSTR* argv)
	{
		UNREFERENCED_PARAMETER(argc);
		UNREFERENCED_PARAMETER(argv);
	}

	// OnStop
	//
	// Sample control handler for ServiceControl::Stop
	void OnStop(void)
	{
	}
};

class MyService : public Service<MyService>
{
public:

	MyService()=default;

	void OnStart(int argc, svctl::tchar_t** argv)
	{
		UNREFERENCED_PARAMETER(argc);
		UNREFERENCED_PARAMETER(argv);

		//Sleep(5000);

		//std::async(std::launch::async, [=]() { 
		//
		//	while(true) {
		//		svctl::tstring param = m_paramtestsz;
		//		OutputDebugString(param.c_str());
		//		OutputDebugString(_T("\r\n"));
		//		Sleep(100);
		//	}
		//});

	}

	void OnStop(void)
	{
		//Sleep(5000);
		int x = 123;
	}

	BEGIN_CONTROL_HANDLER_MAP(MyService)
		CONTROL_HANDLER_ENTRY(ServiceControl::Stop, OnStop)
	END_CONTROL_HANDLER_MAP()

	BEGIN_PARAMETER_MAP(MyService)
		PARAMETER_ENTRY(_T("TestSz"), m_paramtestsz)
	END_PARAMETER_MAP()

private:

	MyService(const MyService&)=delete;
	MyService& operator=(const MyService&)=delete;

	StringParameter m_paramtestsz { _T("defaultparam") };
};

//////////////////////////////////////
template <class _service>
class ServiceHarness
{
public:
	
	// Constructor
	ServiceHarness()
	{
		// Initialize the SERVICE_STATUS structure to the proper defaults
		memset(&m_status, 0, sizeof(SERVICE_STATUS));
		m_status.dwServiceType = static_cast<DWORD>(ServiceProcessType::Unique);
		m_status.dwCurrentState = static_cast<DWORD>(ServiceStatus::Stopped);
	}

	// Destructor
	~ServiceHarness()=default;

	// Start
	//
	// Starts the service, optionally specifying a variadic set of command line arguments.
	// Arguments can be C-style strings, fundamental data types, or svctl::tstring references
	template <typename... _arguments>
	void Start(LPCTSTR servicename, const _arguments&... arguments)
	{
		// Construct a vector<> for the arguments starting with the service name
		// and recursively invoke one of the variadic overloads until done
		Start(std::vector<svctl::tstring> { servicename }, arguments...);
	}

	DWORD Control(ServiceControl control)
	{
		return Control(control, 0, nullptr);
	}

	// This isn't something a normal client can do ... but useful in the harness
	DWORD Control(ServiceControl control, DWORD eventtype, void* eventdata)
	{
	}

	DWORD Stop(void)
	{
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

		return m_handler(static_cast<DWORD>(ServiceControl::Stop), 0, nullptr, m_context);
	}

	// WaitForStatus
	//
	// Waits for the service to reach a specific status
	bool WaitForStatus(ServiceStatus status, uint32_t timeout = INFINITE)
	{
		std::unique_lock<std::mutex> critsec(m_statuslock);
		return m_statuschanged.wait_until(critsec, std::chrono::system_clock::now() + std::chrono::milliseconds(timeout),
			[=]() { return static_cast<ServiceStatus>(m_status.dwCurrentState) == status; });
	}

	void WaitForStop(void)
	{
		// this really isn't quite right, but works for now
		m_mainthread.join();
	}

	// Status
	//
	// Gets a copy of the current service status
	__declspec(property(get=getStatus)) SERVICE_STATUS Status;
	SERVICE_STATUS getStatus(void) { std::lock_guard<std::mutex> critsec(m_statuslock); return m_status; }

private:

	ServiceHarness(const ServiceHarness&)=delete;
	ServiceHarness& operator=(const ServiceHarness&)=delete;

	// Start (variadic)
	//
	// Recursive variadic function, converts for a fundamental type argument
	template <typename _next, typename... _remaining>
	typename std::enable_if<std::is_fundamental<_next>::value, void>::type
	Start(std::vector<svctl::tstring>& argvector, const _next& next, const _remaining&... remaining)
	{
		argvector.push_back(to_tstring(next));
		Start(argvector, remaining...);
	}

	// Start (variadic)
	//
	// Recursive variadic function, processes an svctl::tstring type argument
	template <typename... _remaining>
	void Start(std::vector<svctl::tstring>& argvector, const svctl::tstring& next, const _remaining&... remaining)
	{
		argvector.push_back(next);
		Start(argvector, remaining...);
	}
	
	// Start (variadic)
	//
	// Recursive variadic function, processes a generic text C-style string pointer
	template <typename... _remaining>
	void Start(std::vector<svctl::tstring>& argvector, const svctl::tchar_t* next, const _remaining&... remaining)
	{
		argvector.push_back(next);
		Start(argvector, remaining...);
	}
	
	// Start (variadic)
	//
	// Final overload in the variadic chain for Start()
	void Start(std::vector<svctl::tstring>& argvector)
	{
		// If the main thread has already been created, the service has already been started
		if(m_mainthread.joinable()) throw svctl::winexception(ERROR_SERVICE_ALREADY_RUNNING);

		// There is an expectation that argv[0] is set to the service name
		if((argvector.size() == 0) || (argvector[0].length() == 0)) throw svctl::winexception(E_INVALIDARG);

		// Define the control handler registration callback for this harness instance
		svctl::register_handler_func registerfunc = ([=](LPCTSTR servicename, LPHANDLER_FUNCTION_EX handler, LPVOID context) -> SERVICE_STATUS_HANDLE 
		{
			_ASSERTE(handler != nullptr);
			UNREFERENCED_PARAMETER(servicename);

			m_handler = handler;					// Store the handler function pointer
			m_context = context;					// Store the handler context pointer

			// Return the address of this harness instance as a pseudo SERVICE_STATUS_HANDLE for the service
			return reinterpret_cast<SERVICE_STATUS_HANDLE>(this);
		});

		// Define the status change callback for this harness instance
		svctl::set_status_func statusfunc = ([=](SERVICE_STATUS_HANDLE handle, LPSERVICE_STATUS status) -> BOOL { 

			std::unique_lock<std::mutex> critsec(m_statuslock);

			// Ensure that the handle provided is actually the address of this harness instance
			_ASSERTE(reinterpret_cast<ServiceHarness*>(handle) == this);
			if(reinterpret_cast<ServiceHarness*>(handle) != this) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }

			m_status = *status;						// Copy the new SERVICE_STATUS
			critsec.unlock();						// Release the critical section
			m_statuschanged.notify_all();			// Notify the status has been changed

			return TRUE;
		});

		// Services are designed to be called on their own thread
		m_mainthread = std::move(std::thread([=]() {

			// Create a copy of the arguments vector local to this thread so it won't be destroyed
			// prematurely, and convert it into an argv-style array of generic text string pointers
			std::vector<svctl::tstring> arguments(argvector);
			std::vector<svctl::tchar_t*> argv;
			for(const auto& arg: arguments) argv.push_back(const_cast<svctl::tchar_t*>(arg.c_str()));
			argv.push_back(nullptr);

			// Create the context for the service using the lambdas defined above and invoke LocalMain()
			svctl::service_context context = { ServiceProcessType::Unique, registerfunc, statusfunc };
			_service::LocalMain<_service>(argv.size() - 1, argv.data(), context);
		}));

		// Wait up to 30 seconds for the service to set SERVICE_START_PENDING before giving up
		if(!WaitForStatus(ServiceStatus::StartPending, 30000)) throw svctl::winexception(ERROR_SERVICE_REQUEST_TIMEOUT);
	}

	// m_context
	//
	// Context pointer registered for the service control handler
	void* m_context = nullptr;

	// m_handler
	//
	// Service control handler callback function pointer
	LPHANDLER_FUNCTION_EX m_handler = nullptr;

	// m_mainthread
	//
	// Main service thread
	std::thread m_mainthread;

	// m_status
	//
	// Current service status
	SERVICE_STATUS m_status;

	// m_statuschanged
	//
	// Condition variable set when service status has changed
	std::condition_variable m_statuschanged;

	// m_statuslock
	//
	// Critical section to serialize access to the SERVICE_STATUS
	std::mutex m_statuslock;
};

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{

#ifdef _DEBUG

	int nDbgFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);	// Get current flags
	nDbgFlags |= _CRTDBG_LEAK_CHECK_DF;						// Enable leak-check
	_CrtSetDbgFlag(nDbgFlags);								// Set the new flags

#endif	// _DEBUG

	//LPTSTR myargv[] = { L"MyService",  L"sweeeet - argument number 2" };

	ServiceHarness<MyService> runner;
	runner.Start(L"MyService", 1, 1.0, 11, svctl::tstring(L"sweet"), 14, L"last");
	bool success = runner.WaitForStatus(ServiceStatus::Running);
	runner.Stop();
	SERVICE_STATUS status = runner.Status;
	runner.WaitForStatus(ServiceStatus::Stopped);
	runner.WaitForStop();	// remove me

	///////// TESTING
	// returns when stopped

	// could do ServiceTable.DispatchLocal();
	// local_dispatcher::Start("MyService");
	// local_dispatcher::Control(handle, SERVICE_CONTROL_PAUSE, 0, nullptr);
	// local_dispatcher::WaitForStop("MyService", "MyService2")
	
	return 0;

	// Manual dispatching with dynamic names
	ServiceTable services = { ServiceTableEntry<MyService>(_T("MyService")), ServiceTableEntry<MinimalService>(IDS_MYSERVICE) };
	services.Dispatch();
}

