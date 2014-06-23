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

#ifndef __SERVICELIB_H_
#define __SERVICELIB_H_
#pragma once

// C Runtime Library
#include <crtdbg.h>
#include <stdint.h>
#include <tchar.h>

// Standard Template Library
#include <array>
#include <condition_variable>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <exception>
#include <string>
#include <thread>
#include <vector>

// Windows API
#include <Windows.h>

#pragma warning(push, 4)
#pragma warning(disable:4127)			// conditional expression is constant

//-----------------------------------------------------------------------------
// GENERAL DECLARATIONS
//
// Namespace: (GLOBAL)
//-----------------------------------------------------------------------------

// SERVICE_ACCEPT_USERMODEREBOOT
//
// Missing from Windows 8.1 SDK
#ifndef SERVICE_ACCEPT_USERMODEREBOOT
#define SERVICE_ACCEPT_USERMODEREBOOT	0x00000800
#endif

// SERVICE_CONTROL_USERMODEREBOOT
//
// Missing from Windows 8.1 SDK
#ifndef SERVICE_CONTROL_USERMODEREBOOT
#define SERVICE_CONTROL_USERMODEREBOOT	0x00000040
#endif

// ::ServiceControl
//
// Strongly typed enumeration of SERVICE_CONTROL_XXXX constants
enum class ServiceControl
{
	Stop						= SERVICE_CONTROL_STOP,
	Pause						= SERVICE_CONTROL_PAUSE,
	Continue					= SERVICE_CONTROL_CONTINUE,
	Interrogate					= SERVICE_CONTROL_INTERROGATE,
	Shutdown					= SERVICE_CONTROL_SHUTDOWN,
	ParameterChange				= SERVICE_CONTROL_PARAMCHANGE,
	NetBindAdd					= SERVICE_CONTROL_NETBINDADD,
	NetBindRemove				= SERVICE_CONTROL_NETBINDREMOVE,
	NetBindEnable				= SERVICE_CONTROL_NETBINDENABLE,
	NetBindDisable				= SERVICE_CONTROL_NETBINDDISABLE,
	DeviceEvent					= SERVICE_CONTROL_DEVICEEVENT,
	HardwareProfileChange		= SERVICE_CONTROL_HARDWAREPROFILECHANGE,
	PowerEvent					= SERVICE_CONTROL_POWEREVENT,
	SessionChange				= SERVICE_CONTROL_SESSIONCHANGE,
	PreShutdown					= SERVICE_CONTROL_PRESHUTDOWN,
	TimeChange					= SERVICE_CONTROL_TIMECHANGE,
	TriggerEvent				= SERVICE_CONTROL_TRIGGEREVENT,
	UserModeReboot				= SERVICE_CONTROL_USERMODEREBOOT,
};

// ::ServiceErrorControl
//
// Strongly typed enumeration of SERVICE_ERROR constants
enum class ServiceErrorControl
{
	Ignore						= SERVICE_ERROR_IGNORE,
	Normal						= SERVICE_ERROR_NORMAL,
	Severe						= SERVICE_ERROR_SEVERE,
	Critical					= SERVICE_ERROR_CRITICAL,
};

// ::ServiceProcessType (Bitmask)
//
// Strongly typed enumeration of service process type flags
enum class ServiceProcessType
{
	Unique						= SERVICE_WIN32_OWN_PROCESS,
	Shared						= SERVICE_WIN32_SHARE_PROCESS,
	Interactive					= SERVICE_INTERACTIVE_PROCESS,
};

// ::ServiceStartType
//
// Strongy typed enumeration of SERVICE_XXX_START constants
enum class ServiceStartType
{
	Automatic					= SERVICE_AUTO_START,
	Manual						= SERVICE_DEMAND_START,
	Disabled					= SERVICE_DISABLED,
};

// ::ServiceStatus
//
// Strongly typed enumeration of SERVICE_XXXX status constants
enum class ServiceStatus
{
	Stopped						= SERVICE_STOPPED,
	StartPending				= SERVICE_START_PENDING,
	StopPending					= SERVICE_STOP_PENDING,
	Running						= SERVICE_RUNNING,
	ContinuePending				= SERVICE_CONTINUE_PENDING,
	PausePending				= SERVICE_PAUSE_PENDING,
	Paused						= SERVICE_PAUSED,
};

// ::ServiceProcessType Bitwise Operators
inline ServiceProcessType operator~(ServiceProcessType lhs) {
	return static_cast<ServiceProcessType>(~static_cast<DWORD>(lhs));
}

inline ServiceProcessType operator&(ServiceProcessType lhs, ServiceProcessType rhs) {
	return static_cast<ServiceProcessType>(static_cast<DWORD>(lhs) & (static_cast<DWORD>(rhs)));
}

inline ServiceProcessType operator|(ServiceProcessType lhs, ServiceProcessType rhs) {
	return static_cast<ServiceProcessType>(static_cast<DWORD>(lhs) | (static_cast<DWORD>(rhs)));
}

inline ServiceProcessType operator^(ServiceProcessType lhs, ServiceProcessType rhs) {
	return static_cast<ServiceProcessType>(static_cast<DWORD>(lhs) ^ (static_cast<DWORD>(rhs)));
}

// ::ServiceProcessType Compound Assignment Operators
inline ServiceProcessType& operator&=(ServiceProcessType& lhs, ServiceProcessType rhs) {
	lhs = lhs & rhs;
	return lhs;
}

inline ServiceProcessType& operator|=(ServiceProcessType& lhs, ServiceProcessType rhs) {
	lhs = lhs | rhs;
	return lhs;
}

inline ServiceProcessType& operator^=(ServiceProcessType& lhs, ServiceProcessType rhs) {
	lhs = lhs ^ rhs;
	return lhs;
}

//-----------------------------------------------------------------------------
// SERVICE TEMPLATE LIBRARY INTERNALS
//
// Namespace: svctl
//-----------------------------------------------------------------------------

namespace svctl {

	// svctl::char_t
	//
	// ANSI string character type
	typedef char			char_t;

	// svctl::tchar_t, svctl::tstring, svctl::to_tstring<>
	//
	// Generic text character, STL string and conversion
#ifndef _UNICODE
	typedef char			tchar_t;
	typedef std::string		tstring;

	template <typename _type>
	inline typename std::enable_if<std::is_fundamental<_type>::value, tstring>::type to_tstring(_type value) 
	{
		return std::to_string(value);
	}
#else
	typedef wchar_t			tchar_t;
	typedef std::wstring	tstring;

	template <typename _type>
	inline typename std::enable_if<std::is_fundamental<_type>::value, tstring>::type to_tstring(_type value) 
	{
		return std::to_wstring(value);
	}
#endif

	// svctl::register_handler_func
	//
	// Function used to register a service's control handler callback function
	typedef std::function<SERVICE_STATUS_HANDLE(LPCTSTR, LPHANDLER_FUNCTION_EX, LPVOID)> register_handler_func;

	// svctl::report_status_func
	//
	// Function used to report a service status to the service control manager
	typedef std::function<void(SERVICE_STATUS&)> report_status_func;

	// svctl::set_status_func
	//
	// Function used to set a service status using the handle returned by the register_handler_func
	typedef std::function<BOOL(SERVICE_STATUS_HANDLE, LPSERVICE_STATUS)> set_status_func;

	// svctl::signal_type
	//
	// Constant used to define the type of signal created by svctl::signal<>
	enum class signal_type
	{
		AutomaticReset	= FALSE,
		ManualReset		= TRUE,
	};

	//
	// Global Functions
	//

	// svctl::GetServiceProcessType
	//
	// Reads the service process type bitmask from the registry
	ServiceProcessType GetServiceProcessType(const tchar_t* name);

	//
	// Exception Classes
	//

	// svctl::winexception
	//
	// specialization of std::exception for Win32 error codes
	class winexception : public std::exception
	{
	public:
		
		// Instance Constructors
		explicit winexception(DWORD result);
		explicit winexception(HRESULT hresult) : winexception(static_cast<DWORD>(hresult)) {}
		winexception() : winexception(GetLastError()) {}

		// Destructor
		virtual ~winexception()=default;

		// code
		//
		// Exposes the Win32 error code used to construct the exception
		DWORD code() const { return m_code; }

		// std::exception::what
		//
		// Exposes a string-based representation of the exception (ANSI only)
		virtual const char_t* what() const { return m_what.c_str(); }

	private:

		// m_code
		//
		// Underlying error code
		DWORD m_code;

		// m_what
		//
		// Exception message string derived from the Win32 error code
		std::string m_what;
	};

	//
	// Primitive Classes
	//

	// svctl::resstring
	//
	// Implements a tstring loaded from the module's string table
	class resstring : public tstring
	{
	public:

		// Instance Constructors
		explicit resstring(const tchar_t* str) : tstring(str) {}
		explicit resstring(const tstring& str) : tstring(str) {}
		explicit resstring(unsigned int id) : resstring(id, GetModuleHandle(nullptr)) {}
		resstring(unsigned int id, HINSTANCE instance) : tstring(GetResourceString(id, instance)) {}

	private:

		// GetResourceString
		//
		// Gets a read-only constant string pointer for a specific resource string
		static const tstring GetResourceString(unsigned int id, HINSTANCE instance);
	};

	// svctl::signal
	//
	// Wrapper around an unnamed Win32 Event synchronization object
	template<signal_type _type>
	class signal
	{
	public:

		// Constructors
		signal() : signal(false) {}
		signal(bool signaled)
		{
			// Create the underlying Win32 event object
			m_handle = CreateEvent(nullptr, static_cast<BOOL>(_type), (signaled) ? TRUE : FALSE, nullptr);
			if(m_handle == nullptr) throw winexception();
		}

		// Destructor
		~signal() { CloseHandle(m_handle); }

		// operator HANDLE()
		//
		// Converts this instance into a HANDLE
		operator HANDLE() const { return m_handle; }

		// Pulse
		//
		// Releases any threads waiting on this event object to be signaled
		void Pulse(void) const { if(!PulseEvent(m_handle)) throw winexception(); }

		// Reset
		//
		// Resets the event to a non-signaled state
		void Reset(void) const { if(!ResetEvent(m_handle)) throw winexception(); }

		// Set
		//
		// Sets the event to a signaled state
		void Set(void) const { if(!SetEvent(m_handle)) throw winexception(); }

	private:

		signal(const signal&)=delete;
		signal& operator=(const signal&)=delete;

		// m_handle
		//
		// Event kernel object handle
		HANDLE m_handle;
	};

	//
	// Service Classes
	//

	// svctl::control_handler
	//
	// Base class for all derived service control handlers
	class __declspec(novtable) control_handler
	{
	public:

		// Destructor
		virtual ~control_handler()=default;

		// Invoke
		//
		// Invokes the control handler
		virtual DWORD Invoke(void* instance, DWORD eventtype, void* eventdata) const = 0;

		// Control
		//
		// Gets the control code registered for this handler
		__declspec(property(get=getControl)) ServiceControl Control;
		ServiceControl getControl(void) const { return m_control; }

	protected:

		// Constructor
		control_handler(ServiceControl control) : m_control(control) {}

	private:

		control_handler(const control_handler&)=delete;
		control_handler& operator=(const control_handler&)=delete;

		// m_control
		//
		// ServiceControl code registered for this handler
		const ServiceControl m_control;
	};

	// svctl::control_handler_table
	//
	// Typedef to clean up this declaration.  The control handler table is implemented
	// as a vector of unique pointers to control_handler instances ...
	typedef std::vector<std::unique_ptr<svctl::control_handler>> control_handler_table;

	// svctl::service_table_entry
	//
	// Defines a name and entry point for Service-derived class
	class service_table_entry
	{
	public:

		// Copy Constructor
		service_table_entry(const service_table_entry&)=default;

		// Assignment Operator
		service_table_entry& operator=(const service_table_entry&)=default;

		// SERVICE_TABLE_ENTRY typecasting operator
		operator SERVICE_TABLE_ENTRY() const { return { const_cast<tchar_t*>(m_name.c_str()), m_servicemain }; }

		// Name
		//
		// Gets the service name
		__declspec(property(get=getName)) const tchar_t* Name;
		const tchar_t* getName(void) const { return m_name.c_str(); }

		// ServiceMain
		//
		// Gets the address of the service::ServiceMain function
		__declspec(property(get=getServiceMain)) const LPSERVICE_MAIN_FUNCTION ServiceMain;
		const LPSERVICE_MAIN_FUNCTION getServiceMain(void) const { return m_servicemain; }

	protected:

		// Instance constructors
		service_table_entry(const tchar_t* name, const LPSERVICE_MAIN_FUNCTION servicemain) : 
			m_name(name), m_servicemain(servicemain) {}
		service_table_entry(tstring name, const LPSERVICE_MAIN_FUNCTION servicemain) : 
			m_name(name), m_servicemain(servicemain) {}

	private:

		// m_name
		//
		// The service name
		tstring m_name;

		// m_servicemain
		//
		// The service ServiceMain() static entry point
		LPSERVICE_MAIN_FUNCTION m_servicemain;
	};

	// svctl::parameter_base
	//
	// Base class for template-specific service ss
	class parameter_base
	{
	public:

		// Destructor
		virtual ~parameter_base()=default;

		// Bind
		//
		// Binds the parameter to the parent key and value name
		void Bind(HKEY key, const tchar_t* name);

		// Load
		//
		// Loads the parameter value from the registry
		virtual void Load(void) = 0;

		// Unbind
		//
		// Unbinds the parameter
		void Unbind(void);

	protected:

		// Constructor
		parameter_base()=default;

		// IsBound
		//
		// Determines if the parameter has been bound to the registry
		bool IsBound(void);

		// ReadValue<trivial>
		//
		// Generic version of ReadValue for trivial data types
		template <typename _type> _type ReadValue(DWORD format)
		{
			static_assert(std::is_trivial<_type>::value, "data type is not trivial; ReadValue<> must be specialized");

			_type		value;							// Value to be read from registry
			DWORD		length = sizeof(_type);			// Length of the value buffer

			// Attempt to read the binary value from the registry, set data to zeros on failure
			LONG result = RegGetValue(m_key, nullptr, m_name.c_str(), format | RRF_ZEROONFAILURE, nullptr, &value, &length);
			if(result != ERROR_SUCCESS) throw winexception(result);

			return value;
		}

		// ReadValue<tstring>
		//
		// Specialization of ReadValue<> for REG_SZ / REG_EXPAND_SZ
		template <> tstring ReadValue<tstring>(DWORD format)
		{
			DWORD length = 0;

			// Get the length of the buffer required to hold the string
			LONG result = RegGetValue(m_key, nullptr, m_name.c_str(), format, nullptr, nullptr, &length);
			if(result != ERROR_SUCCESS) throw winexception(result);

			// Allocate a local std::vector<> as the backing storage and read the value from the registry
			std::vector<uint8_t> buffer(length);
			result = RegGetValue(m_key, nullptr, m_name.c_str(), format | RRF_ZEROONFAILURE, nullptr, buffer.data(), &length);
			if(result != ERROR_SUCCESS) throw winexception(result);

			// Convert the registry value into a tstring instance
			return tstring(reinterpret_cast<tchar_t*>(buffer.data()));
		}

		// ReadValue<std::vector<tstring>>
		//
		// Specialization of ReadValue<> for REG_MULTI_SZ
		template <> std::vector<tstring> ReadValue<std::vector<tstring>>(DWORD format)
		{
			DWORD length = 0;

			// Get the length of the buffer required to hold the string array
			LONG result = RegGetValue(m_key, nullptr, m_name.c_str(), format, nullptr, nullptr, &length);
			if(result != ERROR_SUCCESS) throw winexception(result);

			// Allocate a local std::vector<> as the backing storage and read the value from the registry
			std::vector<uint8_t> buffer(length);
			result = RegGetValue(m_key, nullptr, m_name.c_str(), format | RRF_ZEROONFAILURE, nullptr, buffer.data(), &length);
			if(result != ERROR_SUCCESS) throw winexception(result);

			// Create a collection of tstring objects, one for each string in the returned array
			std::vector<tstring> value;
			const tchar_t* current = reinterpret_cast<const tchar_t*>(buffer.data());
			while((current) && (*current)) {

				value.push_back(tstring(current));
				current += _tcslen(current) + 1;
			}

			return value;
		}

		// m_key
		//
		// Bound registry parent key handle
		HKEY m_key = nullptr;

		// m_lock
		//
		// Synchronization object
		std::recursive_mutex m_lock;

		// m_name
		//
		// Bound registry value name
		tstring m_name;

	private:

		parameter_base(const parameter_base&)=delete;
		parameter_base& operator=(const parameter_base&)=delete;
	};

	// svctl::parameter
	//
	// Service parameter template class
	//
	//	_type		- Value type
	//	_format		- Format flags to pass into RegGetValue
	//	_inittype	- Type used with an initializer list
	//	_zeroinit	- Flag that the type can be zero-initialized by default

	template<typename _type, DWORD _format, typename _inittype = _type, bool _zeroinit = std::is_trivial<_type>::value>
	class parameter : public parameter_base
	{
	public:

		// Not intended for use with pointer or reference data types
		static_assert(!std::is_pointer<_type>::value, "Service parameters cannot be pointer types");
		static_assert(!std::is_reference<_type>::value, "Service parameters cannot be reference types");

		// Constructors
		parameter() { if(_zeroinit) memset(&m_value, 0, sizeof(_type)); }
		explicit parameter(_inittype defvalue) : m_value({defvalue}) {}
		
		// TODO: This does not work in Visual C++ 2013, appears to be a bug in the compiler that 
		// prevents using an initializer_list as a non-static member variable initializer
		//
		//parameter(std::initializer_list<_inittype> init) : m_value(init) {}

		// Destructor
		virtual ~parameter()=default;

		// typecasting operator
		operator _type()
		{
			std::lock_guard<std::recursive_mutex> critsec(m_lock);
			return m_value;
		}

		// IsDefaulted
		//
		// Flag if the value has been defaulted or if it has been read from the registry
		__declspec(property(get=getIsDefaulted)) bool IsDefaulted;
		bool getIsDefaulted(void) const { std::lock_guard<std::recursive_mutex> critsec(m_lock); return m_defaulted; }

	private:

		parameter(const parameter&)=delete;
		parameter& operator=(const parameter&)=delete;

		// Load (svctl::parameter_base)
		//
		// Invoked in respose to a SERVICE_CONTROL_PARAM_CHANGE; loads the value
		virtual void Load(void)
		{
			std::lock_guard<std::recursive_mutex> critsec(m_lock);
			if(!IsBound()) return;

			try { 
			
				// Attempt to read the value from the registry, and if successful clear defaulted flag
				m_value = parameter_base::ReadValue<_type>(_format);
				m_defaulted = false;
			}
			
			catch(...) { /* DO NOTHING ON REGISTRY EXCEPTION */ }
		}

		// m_defaulted
		//
		// Flag if the parameter is using the default value
		bool m_defaulted = true;

		// m_value
		//
		// Cached parameter value
		_type m_value;
	};

	// svctl::service_context
	//
	// Service runtime context information provided to ServiceMain to
	// allow for differences in service vs. local application model
	struct service_context
	{
		// ProcessType
		//
		// Defines the service process type (unique/shared)
		ServiceProcessType ProcessType;

		// RegisterHandlerFunc
		//
		// Function used by the service to register the control handler
		register_handler_func RegisterHandlerFunc;

		// SetStatusFunc
		//
		// Function used by the service to set status
		set_status_func SetStatusFunc;
	};

	// svctl::service
	//
	// Primary service base class
	class service
	{
	public:

		// Destructor
		virtual ~service()=default;

	protected:
		
		// Instance Constructor
		service()=default;

		// Continue
		//
		// Continues the service from a paused state
		DWORD Continue(void);

		// IterateParameters
		//
		// Iterates over the collection of parameters and executes a function against each
		virtual void IterateParameters(std::function<void(const tstring& name, parameter_base& param)> func);

		// LocalMain
		//
		// Entry point when the service is executed as an application
		template <class _derived>
		static void LocalMain(DWORD argc, LPTSTR* argv, const service_context& context)
		{
			_ASSERTE(argc);					// Service name = argv[0]

			// Create an instance of the derived service class and invoke ServiceMain() with specified context
			std::unique_ptr<service> instance = std::make_unique<_derived>();
			instance->Main(static_cast<int>(argc), argv, context);
		}

		// OnStart
		//
		// Invoked when the service is started; must be implemented in the service
		virtual void OnStart(int argc, LPTSTR* argv) = 0;

		// Pause
		//
		// Pauses the service
		DWORD Pause(void);

		// ReloadParameters
		//
		// Reloads all of the bound service parameter values
		void ReloadParameters(void);

		// ServiceMain
		//
		// Service entry point, specific for the derived class object
		template <class _derived>
		static void WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
		{
			_ASSERTE(argc);					// Service name = argv[0]

			// When running as a regular service, the process type is read from the registry and the
			// standard Win32 service API functions are used for registration and status reporting
			service_context context = { GetServiceProcessType(argv[0]), ::RegisterServiceCtrlHandlerEx, ::SetServiceStatus };

			// Create an instance of the derived service class and invoke ServiceMain()
			std::unique_ptr<service> instance = std::make_unique<_derived>();
			instance->Main(static_cast<int>(argc), argv, context);
		}

		// Stop
		//
		// Stops the service
		DWORD Stop(void) { return Stop(ERROR_SUCCESS, ERROR_SUCCESS); }
		DWORD Stop(DWORD win32exitcode, DWORD serviceexitcode);

		// Handlers
		//
		// Gets the collection of service-specific control handlers
		__declspec(property(get=getHandlers)) const control_handler_table& Handlers;
		virtual const control_handler_table& getHandlers(void) const;

	private:

		service(const service&)=delete;
		service& operator=(const service&)=delete;

		// PENDING_CHECKPOINT_INTERVAL
		//
		// Interval at which the pending status thread will report progress
		const uint32_t PENDING_CHECKPOINT_INTERVAL = 1000;

		// PENDING_WAIT_HINT
		//
		// Standard wait hint used when a pending status has been set
		const uint32_t PENDING_WAIT_HINT = 2000;

		// STARTUP_WAIT_HINT
		//
		// Wait hint used during the initial service START_PENDING status
		const uint32_t STARTUP_WAIT_HINT = 5000;

		// ControlHandler
		//
		// Service control request handler method
		DWORD ControlHandler(ServiceControl control, DWORD eventtype, void* eventdata);

		// ServiceMain
		//
		// Service entry point
		void Main(int argc, tchar_t** argv, const service_context& context);

		// SetNonPendingStatus
		//
		// Sets a non-pending status
		void SetNonPendingStatus(ServiceStatus status) { SetNonPendingStatus(status, ERROR_SUCCESS, ERROR_SUCCESS); }
		void SetNonPendingStatus(ServiceStatus status, uint32_t win32exitcode, uint32_t serviceexitcode);

		// SetPendingStatus
		//
		// Sets an auto-checkpoint pending status
		void SetPendingStatus(ServiceStatus status);

		// SetStatus
		//
		// Sets a new service status
		void SetStatus(ServiceStatus status) { SetStatus(status, ERROR_SUCCESS, ERROR_SUCCESS); }
		void SetStatus(ServiceStatus status, uint32_t win32exitcode) { SetStatus(status, win32exitcode, ERROR_SUCCESS); }
		void SetStatus(ServiceStatus status, uint32_t win32exitcode, uint32_t serviceexitcode);

		// AcceptedControls
		//
		// Gets what control codes the service will accept
		__declspec(property(get=getAcceptedControls)) DWORD AcceptedControls;
		DWORD getAcceptedControls(void);

		// m_status
		//
		// Current service status
		ServiceStatus m_status = ServiceStatus::Stopped;

		// m_statusexception
		//
		// Holds any exception thrown by a status worker thread
		std::exception_ptr m_statusexception;

		// m_statusfunc
		//
		// Function pointer used to report an updated service status
		report_status_func m_statusfunc;
 
		// m_statuslock;
		//
		// Synchronization object for status updates
		std::recursive_mutex m_statuslock;

		// m_statussignal
		//
		// Signal (event) object used to stop a pending status thread
		signal<signal_type::ManualReset> m_statussignal;

		// m_statusworker
		//
		// Pending status worker thread
		std::thread m_statusworker;

		// m_stopsignal
		//
		// Signal indicating that SERVICE_CONTROL_STOP has been triggered
		signal<signal_type::ManualReset> m_stopsignal;
	};

	// svctl::service_harness
	//
	// Test harness to execute a service as an application
	class service_harness
	{
	public:
	
		// Constructor / Destructor
		service_harness();
		virtual ~service_harness();

		// Continue
		//
		// Sends ServiceControl::Continue and waits for ServiceStatus::Running
		void Continue(void);

		// Pause
		//
		// Sends ServiceControl::Pause and waits for ServiceStatus::Paused
		void Pause(void);

		// SendControl
		//
		// Sends a control code to the service
		DWORD SendControl(ServiceControl control) { return SendControl(control, 0, nullptr); }
		DWORD SendControl(ServiceControl control, DWORD eventtype, void* eventdata);
		
		// Start (string)
		//
		// Starts the service, optionally specifying a variadic set of command line arguments.
		// (Arguments can be C-style strings, fundamental data types, or tstring references)
		template <typename... _arguments>
		void Start(LPCTSTR servicename, const _arguments&... arguments)
		{
			// Construct a vector<> for the arguments starting with the service name
			// and recursively invoke one of the variadic overloads until done
			std::vector<tstring> argvector { servicename };
			Start(argvector, arguments...);
		}

		// Start (resource id)
		//
		// Starts the service, optionally specifying a variadic set of command line arguments.
		// (Arguments can be C-style strings, fundamental data types, or tstring references)
		template <typename... _arguments>
		void Start(unsigned int servicename, const _arguments&... arguments)
		{
			// Construct a vector<> for the arguments starting with the service name
			// and recursively invoke one of the variadic overloads until done
			std::vector<tstring> argvector { resstring(servicename) };
			Start(argvector, arguments...);
		}

		// Stop
		//
		// Stops the service; should be called rather than SendControl()
		// as this also waits for the main thread and resets the status
		void Stop(void);

		// WaitForStatus
		//
		// Waits for the service to reach the specified status
		bool WaitForStatus(ServiceStatus status, uint32_t timeout = INFINITE);

		// CanContinue
		//
		// Determines if the service can be continued
		__declspec(property(get=getCanContinue)) bool CanContinue;
		bool getCanContinue(void);

		// CanPause
		//
		// Determines if the service can be paused
		__declspec(property(get=getCanPause)) bool CanPause;
		bool getCanPause(void);

		// CanStop
		//
		// Determines if the service can be stopped
		__declspec(property(get=getCanStop)) bool CanStop;
		bool getCanStop(void);

		// Status
		//
		// Gets a copy of the current service status
		__declspec(property(get=getStatus)) SERVICE_STATUS Status;
		SERVICE_STATUS getStatus(void) { std::lock_guard<std::mutex> critsec(m_statuslock); return m_status; }

	protected:

		// LaunchService
		//
		// Invokes the derived service class' LocalMain() entry point
		virtual void LaunchService(int argc, LPTSTR* argv, const service_context& context) = 0;

	private:

		service_harness(const service_harness&)=delete;
		service_harness& operator=(const service_harness&)=delete;

		// ServiceControlAccepted (static)
		//
		// Checks a ServiceControl against a SERVICE_ACCEPTS_XXXX mask
		static bool ServiceControlAccepted(ServiceControl control, DWORD mask);

		// Start (variadic)
		//
		// Recursive variadic function, converts for a fundamental type argument
		template <typename _next, typename... _remaining>
		typename std::enable_if<std::is_fundamental<_next>::value, void>::type
		Start(std::vector<tstring>& argvector, const _next& next, const _remaining&... remaining)
		{
			argvector.push_back(to_tstring(next));
			Start(argvector, remaining...);
		}

		// Start (variadic)
		//
		// Recursive variadic function, processes an tstring type argument
		template <typename... _remaining>
		void Start(std::vector<tstring>& argvector, const tstring& next, const _remaining&... remaining)
		{
			argvector.push_back(next);
			Start(argvector, remaining...);
		}
	
		// Start (variadic)
		//
		// Recursive variadic function, processes a generic text C-style string pointer
		template <typename... _remaining>
		void Start(std::vector<tstring>& argvector, const tchar_t* next, const _remaining&... remaining)
		{
			argvector.push_back(next);
			Start(argvector, remaining...);
		}
	
		// Start
		//
		// Final overload in the variadic chain for Start()
		void Start(std::vector<tstring>& argvector);

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

} // namespace svctl

//-----------------------------------------------------------------------------
// SERVICE TEMPLATE LIBRARY
//
// Namespace: (GLOBAL)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// ::ServiceException
//
// Global namespace alias for svctl::winexception

using ServiceException = svctl::winexception;

//-----------------------------------------------------------------------------
// ::ServiceControlHandler<>
//
// Specialization of the svctl::control_handler class that allows the derived
// class to register it's own member functions as handler callbacks

template<class _derived>
class ServiceControlHandler : public svctl::control_handler
{
private:

	// void_handler, void_handler_ex
	//
	// Control handlers that always return ERROR_SUCCESS when invoked
	typedef void(_derived::*void_handler)(void);
	typedef void(_derived::*void_handler_ex)(DWORD, void*);

	// result_handler, result_handler_ex
	//
	// Control handlers that need to return a DWORD result code
	typedef DWORD(_derived::*result_handler)(void);
	typedef DWORD(_derived::*result_handler_ex)(DWORD, void*);

public:

	// Instance Constructors
	ServiceControlHandler(ServiceControl control, void_handler func) :
		control_handler(control), m_void_handler(std::bind(func, std::placeholders::_1)) {}
	ServiceControlHandler(ServiceControl control, void_handler_ex func) :
		control_handler(control), m_void_handler_ex(std::bind(func, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)) {}
	ServiceControlHandler(ServiceControl control, result_handler func) :
		control_handler(control), m_result_handler(std::bind(func, std::placeholders::_1)) {}
	ServiceControlHandler(ServiceControl control, result_handler_ex func) :
		control_handler(control), m_result_handler_ex(std::bind(func, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)) {}

	// Destructor
	virtual ~ServiceControlHandler()=default;

	// Invoke (svctl::control_handler)
	//
	// Invokes the control handler set during handler construction
	virtual DWORD Invoke(void* instance, DWORD eventtype, void* eventdata) const
	{
		// At least one of these is assumed to have been set during construction
		_ASSERTE(m_void_handler || m_void_handler_ex || m_result_handler || m_result_handler_ex);
		
		// Generally control handlers do not need to return a status, default to ERROR_SUCCESS
		DWORD result = ERROR_SUCCESS;

		// Cast the void instance pointer provided back into a derived-class specific pointer
		_derived* derived = reinterpret_cast<_derived*>(instance);

		// One and only one of the member std::function instance should be set
		if(m_void_handler) m_void_handler(derived);
		else if(m_void_handler_ex) m_void_handler_ex(derived, eventtype, eventdata);
		else if(m_result_handler) result = m_result_handler(derived);
		else if(m_result_handler_ex) result = m_result_handler_ex(derived, eventtype, eventdata);
		else throw ServiceException(E_UNEXPECTED);
		
		return result;
	}	

private:

	ServiceControlHandler(const ServiceControlHandler&)=delete;
	ServiceControlHandler& operator=(const ServiceControlHandler&)=delete;

	// m_void_handler
	//
	// Set when a void_handler member function is provided during construction
	const std::function<void(_derived*)> m_void_handler;

	// m_void_handler_ex
	//
	// Set when a void_handler_ex member function is provided during construction
	const std::function<void(_derived*, DWORD, void*)> m_void_handler_ex;

	// m_result_handler
	//
	// Set when a result_handler function is provided during construction
	const std::function<DWORD(_derived*)> m_result_handler;

	// m_result_handler_ex
	//
	// Set when a result_handler_ex function is provided during construction
	const std::function<DWORD(_derived*, DWORD, void*)> m_result_handler_ex;
};

//-----------------------------------------------------------------------------
// ::ServiceTableEntry<>
//
// Template version of svctl::service_table_entry

template <class _derived>
struct ServiceTableEntry : public svctl::service_table_entry
{
	// Instance constructors
	explicit ServiceTableEntry(LPCTSTR name) : 
		service_table_entry(name, &svctl::service::ServiceMain<_derived>) {}
	explicit ServiceTableEntry(unsigned int nameres) : 
		service_table_entry(svctl::resstring(nameres), &svctl::service::ServiceMain<_derived>) {}
	explicit ServiceTableEntry(unsigned int nameres, HINSTANCE instance) : 
		service_table_entry(svctl::resstring(nameres, instance), &svctl::service::ServiceMain<_derived>) {}
};

//-----------------------------------------------------------------------------
// ::ServiceTable
//
// A collection of services to be dispatched to the service control manager
// for the running service process

class ServiceTable : private std::vector<svctl::service_table_entry>
{
public:

	// Constructors
	ServiceTable()=default;
	ServiceTable(const std::initializer_list<svctl::service_table_entry> init) : vector(init) {}

	// Subscript operators
	svctl::service_table_entry& operator[](size_t index) { return vector::operator[](index); }
	const svctl::service_table_entry& operator[](size_t index) const { return vector::operator[](index); }

	// Add
	//
	// Inserts a new entry into the collection
	void Add(const svctl::service_table_entry& item) { vector::push_back(item); }

	// Dispatch
	//
	// Dispatches the service table to the service control manager
	int Dispatch(void);

private:

	ServiceTable(const ServiceTable&)=delete;
	ServiceTable& operator=(const ServiceTable&)=delete;
};

//-----------------------------------------------------------------------------
// ::ServiceHarness<>
//
// Template version of svctl::service_harness

template <class _service>
class ServiceHarness : public svctl::service_harness
{
public:

	// Constructor / Destructor
	ServiceHarness()=default;
	virtual ~ServiceHarness()=default;

private:

	ServiceHarness(const ServiceHarness&)=delete;
	ServiceHarness& operator=(const ServiceHarness&)=delete;

	// LaunchService (service_harness)
	//
	// Launches the derived service by invoking it's LocalMain entry point
	virtual void LaunchService(int argc, LPTSTR* argv, const svctl::service_context& context)
	{
		_service::LocalMain<_service>(argc, argv, context);
	}
};

//-----------------------------------------------------------------------------
// ::Service<>
//
// Template version of svctl::service

template <class _derived>
class Service : public svctl::service
{
friend struct ServiceTableEntry<_derived>;
friend class ServiceHarness<_derived>;
public:

	// Constructor / Destructor
	Service()=default;
	virtual ~Service()=default;

protected:

	// BinaryParameter
	//
	// Generic REG_BINARY parameter for any trivial data type
	template <class _type>
	using BinaryParameter = svctl::parameter<_type, RRF_RT_REG_BINARY>;

	// DWordParameter
	//
	// 32-bit unsigned integer parameter
	using DWordParameter = svctl::parameter<uint32_t, RRF_RT_DWORD>;

	// MultiStringParameter
	//
	// Vector of std:basic_string<tchar_t> parameters
	using MultiStringParameter = svctl::parameter<std::vector<svctl::tstring>, RRF_RT_REG_MULTI_SZ, svctl::tstring>;

	// QWordParameter
	//
	// 64-bit unsigned integer parameter
	using QWordParameter = svctl::parameter<uint64_t, RRF_RT_QWORD>;

	// StringParameter
	//
	// std::basic_string<tchar_t> parameter
	using StringParameter = svctl::parameter<svctl::tstring, RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ>;
	
private:

	Service(const Service&)=delete;
	Service& operator=(const Service&)=delete;
};

// CONTROL_HANDLER_MAP
//
// Used to declare the getHandlers virtual function implementation for the service.
// PAUSE, CONTINUE and STOP handlers will be invoked asynchronously via a pooled thread
// since they involve an automatic pending state change.  Any other handlers are invoked
// inline from HandlerEx in the order that they are declared, it is up to the service
// implementation to process them and return as promptly as possible.  Custom control codes
// are supported but must fall in the range of 128 through 255 (See HandlerEx on MSDN)
//
// Handler functions must be nonstatic member functions that adhere to one of the following
// four function signatures.  An implicit ERROR_SUCCESS (0) is returned on behalf of the
// handler when a "void" version has been selected.
//
//		void	MyHandler(void)
//		void	MyHandler(DWORD eventtype, void* eventdata)
//		DWORD	MyHandler(void)
//		DWORD	MyHandler(DWORD eventtype, void* eventdata)
//
// A dummy handler for SERVICE_CONTROL_INTERROGATE is added to allow for a blank handler
// map to compile without errors, however this method will never be called as this control
// is blocked by the HandlerEx implementation.
//
// Sample usage:
//
//	BEGIN_CONTROL_HANDLER_MAP(MyService)
//		CONTROL_HANDLER_ENTRY(ServiceControl::Stop, OnStop)
//		CONTROL_HANDLER_ENTRY(ServiceControl::ParamChange, OnParameterChange)
//		CONTROL_HANDLER_ENTRY(200, OnMyCustomCommand)
//	END_CONTROL_HANDLER_MAP()
//
#define BEGIN_CONTROL_HANDLER_MAP(_class) \
	typedef _class __control_map_class; \
	void __null_handler##_class(void) { return; } \
	const svctl::control_handler_table& getHandlers(void) const \
	{ \
		static std::unique_ptr<svctl::control_handler> handlers[] = { \
		std::make_unique<ServiceControlHandler<__control_map_class>>(ServiceControl::Interrogate, &__control_map_class::__null_handler##_class),

#define CONTROL_HANDLER_ENTRY(_control, _func) \
		std::make_unique<ServiceControlHandler<__control_map_class>>(static_cast<ServiceControl>(_control), &__control_map_class::_func),

#define END_CONTROL_HANDLER_MAP() \
		}; \
		static svctl::control_handler_table table { std::make_move_iterator(std::begin(handlers)), std::make_move_iterator(std::end(handlers)) }; \
		return table; \
	}

// PARAMETER_MAP
//
// Used to declare the IterateParameters virtual function implementation for the service.
// The map entry defines what the name of the parameter registry value will be and indicates
// what svctl::parameter<> member variable is to be bound to that registry value.
//
// Local module resource identifiers can also be used in lieu of hard-coding the value name
//
// Sample usage:
//
//	BEGIN_PARAMETER_MAP(MyService)
//		PARAMETER_ENTRY(_T("TestExpandSz"), m_expandsz)
//		PARAMETER_ENTRY(IDS_MYDWORD, m_mydword)
//	END_PARAMETER_MAP()
//
//  StringParameter				m_expandsz { _T("defaultstring") };
//	DWordParameter				m_mydword = 0;
//	BinaryParameter<mystruct>	m_binparam;
//
#define BEGIN_PARAMETER_MAP(_class) \
	virtual void IterateParameters(std::function<void(const svctl::tstring& name, svctl::parameter_base& param)> func) {

#define PARAMETER_ENTRY(_name, _var) \
	func(svctl::resstring(_name), _var);

#define END_PARAMETER_MAP() \
	}

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __SERVICELIB_H_
