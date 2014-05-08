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
#include <process.h>
#include <stdint.h>
#include <stdlib.h>
#include <tchar.h>

// Standard Template Library
#include <array>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <thread>
#include <vector>

// Windows API
#include <Windows.h>
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

#pragma warning(push, 4)

//-----------------------------------------------------------------------------
// GLOBAL NAMESPACE DECLARATIONS
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

// ::ServiceProcessType
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

//-----------------------------------------------------------------------------
// SERVICE TEMPLATE LIBRARY INTERNALS
//-----------------------------------------------------------------------------

namespace svctl {

	// Forward Declarations - clean these up only list the 
	// ones that actually have to be here
	struct auxiliary_state;
	class auxiliary_state_machine;
	class service;
	class service_config;
	class service_module;
	class service_table_entry;

	//
	// Data Types
	//

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

	// svctl::handler_complete_func
	//
	// Function used to indicate to the main service class that an asyncronous
	// handler call has completed; used to stop automatic pending states
	typedef std::function<void(void)> handler_complete_func;

	// svctl::report_status_func
	//
	// Function used to report a service status to either the service control manager
	// or the local service dispatcher
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
		explicit lock(const critical_section& cs) : m_cs(cs) { m_cs.Enter(); }
		~lock() { m_cs.Leave(); }

	private:

		lock(const lock&)=delete;
		lock& operator=(const lock&)=delete;

		// m_cs
		//
		// Reference to the critical_section object
		const critical_section&	m_cs;
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
	struct __declspec(novtable) auxiliary_state
	{
		virtual void OnInitialize(const tchar_t* servicename) = 0;
	};

	// svctl::auxiliary_state_machine
	//
	// State machine that ties all of the auxiliary classes in a service inheritance
	// chain together dynamically
	class auxiliary_state_machine
	{
	public:

		// Instance Constructor
		//
		auxiliary_state_machine()=default;

		// Initialize
		//
		// Invokes all of the registered OnInitialize() methods
		void Initialize(const tchar_t* servicename);

	protected:

		// RegisterAuxiliaryState
		//
		// Registers a derived class' auxiliary_state interface
		void RegisterAuxiliaryState(struct auxiliary_state* instance);

	private:

		auxiliary_state_machine(const auxiliary_state_machine&)=delete;
		auxiliary_state_machine& operator=(const auxiliary_state_machine&)=delete;

		// m_instances
		//
		// Collection of registered auxiliary classes
		std::vector<auxiliary_state*> m_instances;
	};

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
		virtual DWORD Invoke(void* instance, DWORD eventtype, void* eventdata, 
			const handler_complete_func& oncomplete) = 0;

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

	// svctl::service
	//
	// Base class that implements a single service instance
	class service : virtual private auxiliary_state_machine
	{
	public:

		// Destructor
		//
		virtual ~service()=default;

		// LocalMain
		//
		// Service entry point invoked when running as a normal process
		// TODO: make private? -- screws with unique_ptr in Service<xxxxx>
		void LocalMain(uint32_t argc, tchar_t** argv);

		// ServiceMain
		//
		// Service entry point invoked when running as a service
		// TODO: make private? -- screws with unique_ptr in Service<xxxxx>
		void ServiceMain(uint32_t argc, tchar_t** argv);

	protected:

		// Instance Constructor
		//
		service()=default;
		// TODO: this needs to flesh out, load m_xxxx variables with the important
		// stuff from the static variables, figure that out since I don't want to have
		// a shit ton of accessor functions again like GetServiceName(), GetServiceType(), etc

		///////////////////////////////////
		// GetServiceName
		//
		// Exposes the name of the service
		virtual const tchar_t* GetServiceName(void) const = 0;

		virtual ServiceProcessType GetServiceType(void) const = 0;
		//////////////////////////

		// Initialize
		//
		// Invoked when the service is being initialized; optional
		// TODO: make public?
		virtual DWORD Initialize(uint32_t argc, tchar_t** argv);

		// IsStatusChangePending
		//
		// Determines if a pending status change is in process; could be useful for
		// the derived service to know this when handling certain controls
		bool IsStatusChangePending(void) { return m_statusworker.joinable(); }

		// Run
		//
		// Main service loop; must be implemented in the derived class
		// TODO: make public?
		virtual DWORD Run(void) = 0;

		// Terminate
		//
		// Invoked when the service is being terminated; optional
		// TODO: make public?
		virtual void Terminate(void);

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
		const uint32_t PENDING_WAIT_HINT = 4000;

		// STARTUP_WAIT_HINT
		//
		// Wait hint used during the initial service START_PENDING status
		const uint32_t STARTUP_WAIT_HINT = 30000;

		/////////////////////////////
		// ControlRequest
		//
		// Service control request handler
		uint32_t ControlRequest(uint32_t control, uint32_t type, void* data);

		// ControlRequestCallback
		//
		// Service handler callback procedure registered with service control manager
		static DWORD WINAPI ControlRequestCallback(DWORD control, DWORD type, void* data, void* context);
		////////////////////////////////////

		// SetNonPendingStatus_l
		//
		// Sets a non-pending status; m_cs should be locked before calling
		void SetNonPendingStatus_l(ServiceStatus status) { SetNonPendingStatus_l(status, ERROR_SUCCESS, ERROR_SUCCESS); }
		void SetNonPendingStatus_l(ServiceStatus status, uint32_t win32exitcode, uint32_t serviceexitcode);

		// SetPendingStatus_l
		//
		// Sets an auto-checkpoint pending status; m_cs should be locked before calling
		void SetPendingStatus_l(ServiceStatus status);

		// SetStatus
		//
		// Sets a new service status
		void SetStatus(ServiceStatus status) { SetStatus(status, ERROR_SUCCESS, ERROR_SUCCESS); }
		void SetStatus(ServiceStatus status, uint32_t win32exitcode) { SetStatus(status, win32exitcode, ERROR_SUCCESS); }
		void SetStatus(ServiceStatus status, uint32_t win32exitcode, uint32_t serviceexitcode);

		// m_acceptedcontrols
		//
		// Mask of controls accepted by the service
		/*TODO const*/ uint32_t m_acceptedcontrols;

		// m_name
		//
		// Service name
		/*TODO const */ tstring m_name;

		// m_processtype
		//
		// Parent service process type
		/*TODO const*/ ServiceProcessType m_processtype;

		// m_status
		//
		// Current service status
		ServiceStatus m_status = ServiceStatus::Stopped;

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
	};

	//// svctl::service_config
	////
	//// Defines all of the properties required to configure or install a service
	//// except service name and service type, which are defined as part of service
	////
	//// TODO: where am I going with this?
	//class service_config
	//{
	//public:

	//	//---------------------------------------------------------------------
	//	// Properties

	//	// DisplayName
	//	//
	//	// Gets/sets the service display name
	//	__declspec(property(get=getDisplayName, put=putDisplayName)) const tchar_t* DisplayName;
	//	const tchar_t* getDisplayName(void) const { return m_displayname.c_str(); }
	//	void putDisplayName(const tchar_t* value) { m_displayname = value; }

	//	// ErrorControl
	//	//
	//	// Gets/sets the service error control flag
	//	__declspec(property(get=getErrorControl, put=putErrorControl)) ServiceErrorControl ErrorControl;
	//	ServiceErrorControl getErrorControl(void) const { return m_errorcontrol; }
	//	void putErrorControl(ServiceErrorControl value) { m_errorcontrol = value; }

	//	// StartType
	//	//
	//	// Gets/sets the service start type
	//	__declspec(property(get=getStartType, put=putStartType)) ServiceStartType StartType;
	//	ServiceStartType getStartType(void) const { return m_starttype; }
	//	void putStartType(ServiceStartType value) { m_starttype = value; }

	//private:

	//	// m_displayname
	//	//
	//	// Service display name
	//	tstring	m_displayname;

	//	// m_starttype
	//	//
	//	// Service start type
	//	ServiceStartType m_starttype;

	//	// m_errorcontrol
	//	//
	//	// Service error control flag
	//	ServiceErrorControl m_errorcontrol;
	//};

	// svctl::service_module
	//
	// Base class that implements the module-level service processing code
	class service_module
	{
	public:

		// Instance Constructor
		//
		service_module(const std::initializer_list<service_table_entry>& services) : m_services(services) {}

		// Destructor
		//
		virtual ~service_module()=default;

	protected:

		// Dispatch
		//
		// Dispatches a collection of services to the service control manager
		int Dispatch(const std::vector<service_table_entry>& services);

		// ModuleMain
		//
		// Module entry point function
		int ModuleMain(int argc, tchar_t** argv);

	private:

		service_module(const service_module&)=delete;
		service_module& operator=(const service_module&)=delete;

		// LocalServiceThread
		//
		// Local service dispatcher service execution thread
		static unsigned __stdcall LocalServiceThread(void* arg);

		// StartLocalServiceDispatcher
		//
		// Emulates StartServiceCtrlDispatcher when run as a normal application
		int StartLocalServiceDispatcher(LPSERVICE_TABLE_ENTRY table);

		// m_services
		//
		// Collection of all service_table_entry structures for each service
		const std::vector<service_table_entry> m_services;
	};

	// svctl::service_table_entry
	//
	// holds the static information necessary to dispatch a service class object
	class service_table_entry
	{
	public:

		// Instance Constructor
		//
		service_table_entry(const tchar_t* name, LPSERVICE_MAIN_FUNCTION servicemain, LPSERVICE_MAIN_FUNCTION localmain) :
			m_name(name), m_servicemain(servicemain), m_localmain(localmain) {}

		// LocalServiceEntry
		//
		// Gets a SERVICE_TABLE_ENTRY for use with the local service dispatcher
		__declspec(property(get=getLocalServiceEntry)) SERVICE_TABLE_ENTRY LocalServiceEntry;
		SERVICE_TABLE_ENTRY getLocalServiceEntry(void) const { return SERVICE_TABLE_ENTRY { const_cast<tchar_t*>(m_name), m_localmain }; };

		// ServiceEntry
		//
		// Gets a SERVICE_TABLE_ENTRY for use with the system service dispatcher
		__declspec(property(get=getServiceEntry)) SERVICE_TABLE_ENTRY ServiceEntry;
		SERVICE_TABLE_ENTRY getServiceEntry(void) const { return { const_cast<tchar_t*>(m_name), m_servicemain }; };
	
	private:

		// m_name
		//
		// Service name
		const tchar_t* m_name;

		// m_servicemain
		//
		// Pointer to the service's static ServiceMain() entry point
		LPSERVICE_MAIN_FUNCTION m_servicemain;

		// m_localmain
		//
		// Pointer to the service's static LocalMain() entry point
		LPSERVICE_MAIN_FUNCTION m_localmain;		
	};

} // namespace svctl

//
// GLOBAL NAMESPACE CLASSES
//
// > Public methods that match an API should use the API data type declarations,
//   for example use LPTSTR instead of svctl::tchar_t*
//

// Forward Declarations
//
template<class _derived> class Service;
template<class... _services> class ServiceModule;

///////////////
template<class _derived>
class InlineControlHandler : public svctl::control_handler
{
private:

	// Inline Handler Types
	typedef void(_derived::*noresult_void_handler)(void);
	typedef void(_derived::*noresult_data_handler)(DWORD, void*);
	typedef DWORD(_derived::*result_void_handler)(void);
	typedef DWORD(_derived::*result_data_handler)(DWORD, void*);

public:

	// Instance Constructors
	InlineControlHandler(ServiceControl control, noresult_void_handler func) :
		control_handler(control), m_voidfunc(std::bind(func, std::placeholders::_1)) {}
	InlineControlHandler(ServiceControl control, noresult_data_handler func) :
		control_handler(control), m_datafunc(std::bind(func, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)) {}
	InlineControlHandler(ServiceControl control, result_void_handler func) :
		control_handler(control), m_statusvoidfunc(std::bind(func, std::placeholders::_1)) {}
	InlineControlHandler(ServiceControl control, result_data_handler func) :
		control_handler(control), m_statusdatafunc(std::bind(func, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)) {}

	// Destructor
	virtual ~InlineControlHandler()=default;

	// Invoke (svctl::control_handler)
	//
	// Invokes the control handler
	virtual DWORD Invoke(void* instance, DWORD eventtype, void* eventdata, const svctl::handler_complete_func& oncomplete)
	{
		_ASSERTE(m_voidfunc || m_datafunc || m_statusvoidfunc || m_statusdatafunc);
		
		DWORD result = ERROR_SUCCESS;
		_derived* d = reinterpret_cast<_derived*>(instance);

		if(m_voidfunc) m_voidfunc(d);
		else if(m_datafunc) m_datafunc(d, eventtype, eventdata);
		else if(m_statusvoidfunc) result = m_statusvoidfunc(d);
		else if(m_statusdatafunc) result = m_statusdatafunc(d, eventtype, eventdata);
		else throw svctl::winexception(E_UNEXPECTED);

		// Invoke the completion handler if one has been specified
		if(oncomplete) oncomplete();
		
		return result;
	}	

private:

	InlineControlHandler(const InlineControlHandler&)=delete;
	InlineControlHandler& operator=(const InlineControlHandler&)=delete;

	// m_
	//
	// 
	std::function<void(_derived*)> m_voidfunc;
	std::function<void(_derived*, DWORD, void*)> m_datafunc;
	std::function<DWORD(_derived*)> m_statusvoidfunc;
	std::function<DWORD(_derived*, DWORD, void*)> m_statusdatafunc;
};
///////////////


//-----------------------------------------------------------------------------
// Service<>
//
// Primary service class implementation
template<class _derived>
class Service : public svctl::service
{
public:

	Service()=default;
	virtual ~Service()=default;

	// s_tableentry
	//
	// Container for the static service class data
	static svctl::service_table_entry s_tableentry;

protected:

	// todo: fix the name clash with this and the static member, perhaps
	// this can be gotten from a base service_config class instead, that's planned
	// for service installation statics without having to instantiate the actual
	// service class like the old svctl needed to do
	virtual const svctl::tchar_t* GetServiceName(void) const { return _derived::getServiceName(); }

	virtual ServiceProcessType GetServiceType(void) const { return _derived::getServiceType(); }

private:

	Service(const Service&)=delete;
	Service& operator=(const Service&)=delete;

	// getServiceName
	//
	// Gets the service name
	static const svctl::tchar_t* getServiceName() { return _T("TODOCHANGEME"); }

	static const ServiceProcessType getServiceType() { return ServiceProcessType::Shared; }

	// StaticLocalMain
	//
	// Service class entry point used when running as a normal application
	static void WINAPI StaticLocalMain(DWORD argc, LPTSTR* argv);
	
	// StaticServiceMain
	//
	// Service class entry point when running as a service
	static void WINAPI StaticServiceMain(DWORD argc, LPTSTR* argv);
};

// Service<_derived>::s_tableentry (private, static)
//
// Defines the static service_table_entry information for this service class
template<class _derived>
svctl::service_table_entry Service<_derived>::s_tableentry(_derived::getServiceName(), _derived::StaticServiceMain, _derived::StaticLocalMain);

// Service<_derived>::StaticLocalMain (private, static)
//
// Static entry point for the service when run as a normal process
template<class _derived>
void Service<_derived>::StaticLocalMain(DWORD argc, LPTSTR* argv)
{
	// Construct the derived service class object and invoke the non-static entry point
	std::unique_ptr<svctl::service> instance = std::make_unique<_derived>();
	instance->LocalMain(argc, argv);
}

// Service<_derived>::StaticServiceMain (private, static)
//
// Static entry point for the service when run as a service process
template<class _derived>
void Service<_derived>::StaticServiceMain(DWORD argc, LPTSTR* argv)
{
	// Construct the derived service class object and invoke the non-static entry point
	std::unique_ptr<svctl::service> instance = std::make_unique<_derived>();
	instance->ServiceMain(argc, argv);
}

//-----------------------------------------------------------------------------
// ServiceModule<>
//
// Service process module implementation
template<class... _services>
class ServiceModule : public svctl::service_module
{
public:

	// Instance Constructor
	//
	explicit ServiceModule() : service_module({ (_services::s_tableentry)... }) {}

	// Destructor
	//
	virtual ~ServiceModule()=default;

	// main
	//
	// Entry point for console application modules
	int main(int argc, LPTSTR* argv)
	{
		// Pass the command line arguments into the base class implementation
		return service_module::ModuleMain(argc, argv);
	}

	// WinMain
	//
	// Entry point for Windows application modules
	int WinMain(HINSTANCE instance, HINSTANCE previous, LPTSTR cmdline, int show)
	{
		UNREFERENCED_PARAMETER(instance);
		UNREFERENCED_PARAMETER(previous);
		UNREFERENCED_PARAMETER(cmdline);
		UNREFERENCED_PARAMETER(show);

		// Pass the main() style command line arguments into the base class implementation
		return service_module::ModuleMain(__argc, __targv);
	}

private:

	ServiceModule(const ServiceModule&)=delete;
	ServiceModule& operator=(const ServiceModule&)=delete;
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __SERVICELIB_H_
