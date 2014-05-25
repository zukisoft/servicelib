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
#include <functional>
#include <future>
#include <memory>
#include <exception>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

// Windows API
#include <Windows.h>

#pragma warning(push, 4)

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

	// svctl::tchar_t, svctl::tstring
	//
	// General text character and STL string types
#ifndef _UNICODE
	typedef char			tchar_t;
	typedef std::string		tstring;
#else
	typedef wchar_t			tchar_t;
	typedef std::wstring	tstring;
#endif

	// svctl::report_status_func
	//
	// Function used to report a service status to the service control manager
	typedef std::function<void(SERVICE_STATUS&)> report_status_func;

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

	// svctl::critical_section
	//
	// Win32 CRITICAL_SECTION wrapper class
	class critical_section
	{
	public:

		// Constructor / Destructor
		critical_section() { InitializeCriticalSection(&m_cs); }
		~critical_section() { DeleteCriticalSection(&m_cs); }

		// Enter
		//
		// Enters the critical section
		void Enter(void) const { EnterCriticalSection(&m_cs); }

		// Leave
		//
		// Leaves the critical section
		void Leave(void) const { LeaveCriticalSection(&m_cs); }

	private:

		critical_section(const critical_section&)=delete;
		critical_section& operator=(const critical_section& rhs)=delete;
	
		// m_cs
		//
		// Contained CRITICAL_SECTION synchronization object
		mutable CRITICAL_SECTION m_cs;
	};

	// svctl::lock
	//
	// Provides an automatic enter/leave wrapper around a critical_section
	class lock
	{
	public:

		// Constructor / Destructor
		explicit lock(const critical_section& cs) : m_cs(cs), m_locked(true) { m_cs.Enter(); }
		~lock() { if(m_locked) m_cs.Leave(); }

		// Release
		//
		// Releases the critical section prior to when the object is unwound
		void Release(void) { if(m_locked) m_cs.Leave(); m_locked = false; }

	private:

		lock(const lock&)=delete;
		lock& operator=(const lock&)=delete;

		// m_cs
		//
		// Reference to the critical_section object
		const critical_section&	m_cs;

		// m_locked
		//
		// Flag if the critical section is locked; used to release
		// the critical section before the objectis unwound
		bool m_locked;
	};

	// svctl::resstring
	//
	// Implements a tstring loaded from the module's string table
	class resstring : public tstring
	{
	public:

		// Instance Constructors
		explicit resstring(unsigned int id) : resstring(id, GetModuleHandle(nullptr)) {}
		resstring(unsigned int id, HINSTANCE instance) : tstring(GetResourceString(id, instance)) {}

	private:

		// GetResourceString
		//
		// Gets a read-only constant string pointer for a specific resource string
		static const tchar_t* GetResourceString(unsigned int id, HINSTANCE instance);
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

	// svctl::auxiliary_state
	//
	// Interface used by auxiliary classes that are tied into the service via
	// inheritance of the auxiliary_state_machine
	//struct __declspec(novtable) auxiliary_state
	//{
	//	// OnContinue
	//	//
	//	// Invoked when the derived service is continued
	//	virtual void OnContinue(void) {}   
	//	
	//	// TODO: do I want these to stay defaulted or change this entirely.
	//	// Can probably just do OnStart() and then OnControl() or something

	//	// OnPause
	//	//
	//	// Invoked when the derived service is paused
	//	virtual void OnPause(void) {}

	//	// OnStart
	//	//
	//	// Invoked when the derived service is started
	//	virtual void OnStart(int, tchar_t**) {}

	//	// OnStop
	//	//
	//	// Invoked when the derived service is stopped
	//	virtual void OnStop(void) {}
	//};

	// svctl::auxiliary_state_machine
	//
	// State machine that ties all of the auxiliary classes in a service inheritance
	// chain together dynamically
	//class auxiliary_state_machine
	//{
	//public:

	//	// Instance Constructor
	//	//
	//	auxiliary_state_machine()=default;

	//	// OnContinue
	//	//
	//	// Invokes all of the registered OnContinue() methods
	//	void OnContinue(void) { for(const auto& iterator : m_instances) iterator->OnContinue(); }

	//	// OnPause
	//	//
	//	// Invokes all of the registered OnPause() methods
	//	void OnPause(void) { for(const auto& iterator : m_instances) iterator->OnPause(); }

	//	// OnStart
	//	//
	//	// Invokes all of the registered OnStart() methods
	//	void OnStart(int argc, tchar_t** argv) { 
	//		for(const auto& iterator : m_instances) iterator->OnStart(argc, argv); 
	//	}

	//	// OnStop
	//	//
	//	// Invokes all of the registered OnStop() methods
	//	void OnStop(void) { for(const auto& iterator : m_instances) iterator->OnStop(); }

	//protected:

	//	// RegisterAuxiliaryState
	//	//
	//	// Registers a derived class' auxiliary_state interface
	//	void RegisterAuxiliaryState(struct auxiliary_state* instance);

	//private:

	//	auxiliary_state_machine(const auxiliary_state_machine&)=delete;
	//	auxiliary_state_machine& operator=(const auxiliary_state_machine&)=delete;

	//	// m_instances
	//	//
	//	// Collection of registered auxiliary classes
	//	std::vector<auxiliary_state*> m_instances;
	//};

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

		// todo
		operator SERVICE_TABLE_ENTRY() const { return { const_cast<tchar_t*>(m_name), m_servicemain }; }

		// Name
		//
		// Gets the service name
		__declspec(property(get=getName)) const tchar_t* Name;
		const tchar_t* getName(void) const { return m_name; }

		// ServiceMain
		//
		// Gets the address of the service::ServiceMain function
		__declspec(property(get=getServiceMain)) const LPSERVICE_MAIN_FUNCTION ServiceMain;
		const LPSERVICE_MAIN_FUNCTION getServiceMain(void) const { return m_servicemain; }

	protected:

		// Instance constructor
		service_table_entry(const tchar_t* name, const LPSERVICE_MAIN_FUNCTION servicemain) :
			m_name(name), m_servicemain(servicemain) {}

	private:

		// m_name
		//
		// The service name
		const tchar_t* m_name;

		// m_servicemain
		//
		// The service ServiceMain() static entry point
		LPSERVICE_MAIN_FUNCTION m_servicemain;
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

		// OnStart
		//
		// Invoked when the service is started; must be implemented in the service
		virtual void OnStart(int argc, LPTSTR* argv) = 0;

		// Continue
		//
		// Continues the service from a paused state
		DWORD Continue(void);

		// Pause
		//
		// Pauses the service
		DWORD Pause(void);

		// ServiceMain
		//
		// Service entry point, specific for the derived class object
		template <class _derived>
		static void WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
		{
			std::unique_ptr<service> instance = std::make_unique<_derived>();
			instance->ServiceMain(static_cast<int>(argc), argv);
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
		void ServiceMain(int argc, tchar_t** argv);

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
		DWORD getAcceptedControls(void) const;

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
		// CRITICAL_SECTION synchronization object for status updates
		critical_section m_statuslock;

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

} // namespace svctl

//-----------------------------------------------------------------------------
// SERVICE TEMPLATE LIBRARY
//
// Namespace: (GLOBAL)
//-----------------------------------------------------------------------------

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
		else throw svctl::winexception(E_UNEXPECTED);
		
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
	// Instance constructor
	explicit ServiceTableEntry(const svctl::tchar_t* name) : service_table_entry(name, &svctl::service::ServiceMain<_derived>) {}
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
// ::Service<>
//
// Template version of svctl::service

template <class _derived>
class Service : public svctl::service
{
friend struct ServiceTableEntry<_derived>;
public:

	// Constructor / Destructor
	Service()=default;
	virtual ~Service()=default;

private:

	Service(const Service&)=delete;
	Service& operator=(const Service&)=delete;
};

//-----------------------------------------------------------------------------
// ::ServiceModule<>
//
// Service process module implementation
//
// TODO: Implement

class ServiceModule
{
public:

	// Instance Constructor
	ServiceModule()=default;

	// Destructor
	//
	virtual ~ServiceModule()=default;

	// main
	//
	// Entry point for console application modules
	//int main(int argc, LPTSTR* argv)
	//{
	//	(argc);
	//	(argv);
	//	// Pass the command line arguments into the base class implementation
	//	///return service_module::ModuleMain(argc, argv);
	//	return 0;
	//}

	//// WinMain
	////
	//// Entry point for Windows application modules
	//int WinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
	//{
	//	// Pass the main() style command line arguments into the base class implementation
	//	//return service_module::ModuleMain(__argc, __targv);
	//	return 0;
	//}

private:

	ServiceModule(const ServiceModule&)=delete;
	ServiceModule& operator=(const ServiceModule&)=delete;
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __SERVICELIB_H_
