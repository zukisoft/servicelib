// servicelib.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "servicelib.h"
#include "resource.h"

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

		std::async(std::launch::async, [=]() { 
		
			while(true) {
				svctl::tstring param = m_paramtestsz;
				OutputDebugString(param.c_str());
				OutputDebugString(_T("\r\n"));
				Sleep(100);
			}
		});

	}

	void OnStop(void)
	{
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

	DWORD Stop(void)
	{
		return m_handler(static_cast<DWORD>(ServiceControl::Stop), 0, nullptr, m_context);
	}

	void WaitForStop(void)
	{
		m_mainthread.join();
	}

	// Status
	//
	// Gets the current service status (by value, this is a copy of it)
	__declspec(property(get=getStatus)) SERVICE_STATUS Status;
	SERVICE_STATUS getStatus(void) const { svctl::lock cs(m_lock); return m_status; }

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

		// Reset the signal object that identifies when the service has set it's initial status and we can return
		m_startsignal.Reset();

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

			svctl::lock cs(m_lock);					// Synchronize with harness instance members

			// Ensure that the handle provided is actually the address of this harness instance
			_ASSERTE(reinterpret_cast<ServiceHarness*>(handle) == this);
			if(reinterpret_cast<ServiceHarness*>(handle) != this) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }

			// When the service sets SERVICE_START_PENDING, it's OK to unblock the Start() function
			if(status->dwCurrentState == SERVICE_START_PENDING) m_startsignal.Set();

			// Copy the SERVICE_STATUS structure into the local member variable
			m_status = *status;

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
		if(WaitForSingleObject(m_startsignal, 30000) == WAIT_TIMEOUT) throw svctl::winexception(ERROR_SERVICE_REQUEST_TIMEOUT);
	}

	// m_context
	//
	// Context pointer registered for the service control handler
	void* m_context = nullptr;

	// m_handler
	//
	// Service control handler callback function pointer
	LPHANDLER_FUNCTION_EX m_handler = nullptr;
		
	// m_lock
	//
	// Synchronization object
	svctl::critical_section m_lock;
		
	// m_mainthread
	//
	// Main service thread
	std::thread m_mainthread;

	// m_startsignal
	//
	// Signaled once the service has set an initial StartPending status
	svctl::signal<svctl::signal_type::ManualReset> m_startsignal;

	// m_status
	//
	// Current service status
	SERVICE_STATUS m_status;
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
	Sleep(5000);
	runner.Stop();
	runner.WaitForStop();

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

