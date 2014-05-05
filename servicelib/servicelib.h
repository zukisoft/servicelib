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
#include <vector>

// Windows API
#include <Windows.h>
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

#pragma warning(push, 4)

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

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

enum class ServiceControl;
enum class ServiceErrorControl;
enum class ServiceProcessType;
enum class ServiceStartType;
enum class ServiceStatus;

namespace svctl {

	// Forward Declarations
	//
	struct auxiliary_state;
	class auxiliary_state_machine;
	class resstring;
	class service;
	class service_config;
	class service_module;
	class service_status_manager;
	class service_table_entry;
	class winexception;

	// svctl::char_t, svctl::tchar_t, svctl::tstring
	//
	typedef char			char_t;
#ifndef _UNICODE
	typedef char			tchar_t;
	typedef std::string		tstring;
#else
	typedef wchar_t			tchar_t;
	typedef std::wstring	tstring;
#endif

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

	// svctl::resstring
	//
	// Implements a tstring loaded from the module's string table
	class resstring : public tstring
	{
	public:

		// Instance Constructors
		//
		explicit resstring(unsigned int id) : resstring(id, GetModuleHandle(nullptr)) {}
		resstring(unsigned int id, HINSTANCE instance) : tstring(GetResourceString(id, instance)) {}

	private:

		// GetResourceString
		//
		// Gets a read-only constant string pointer for a specific resource string
		static const tchar_t* GetResourceString(unsigned int id, HINSTANCE instance);
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
		void LocalMain(uint32_t argc, tchar_t** argv);

		// Run
		//
		// Main service loop
		virtual uint32_t Run(uint32_t argc, tchar_t** argv) = 0;

		// ServiceMain
		//
		// Service entry point invoked when running as a service
		void ServiceMain(uint32_t argc, tchar_t** argv);

	protected:

		// Instance Constructor
		//
		service()=default;

		// GetServiceName
		//
		// Exposes the name of the service
		virtual const tchar_t* GetServiceName(void) const = 0;

	private:

		service(const service&)=delete;
		service& operator=(const service&)=delete;

		// ControlRequest
		//
		// Service control request handler
		uint32_t ControlRequest(uint32_t control, uint32_t type, void* data);

		// ControlRequestCallback
		//
		// Service handler callback procedure registered with service control manager
		static DWORD WINAPI ControlRequestCallback(DWORD control, DWORD type, void* data, void* context);

		// m_statusmgr
		//
		// Service status manager instance
		std::unique_ptr<service_status_manager> m_statusmgr;
	};

	// svctl::service_config
	//
	// Defines all of the properties required to configure or install a service
	// except service name and service type, which are defined as part of service
	//
	// TODO: where am I going with this?
	class service_config
	{
	public:

		//---------------------------------------------------------------------
		// Properties

		// DisplayName
		//
		// Gets/sets the service display name
		__declspec(property(get=getDisplayName, put=putDisplayName)) const tchar_t* DisplayName;
		const tchar_t* getDisplayName(void) const { return m_displayname.c_str(); }
		void putDisplayName(const tchar_t* value) { m_displayname = value; }

		// ErrorControl
		//
		// Gets/sets the service error control flag
		__declspec(property(get=getErrorControl, put=putErrorControl)) ServiceErrorControl ErrorControl;
		ServiceErrorControl getErrorControl(void) const { return m_errorcontrol; }
		void putErrorControl(ServiceErrorControl value) { m_errorcontrol = value; }

		// StartType
		//
		// Gets/sets the service start type
		__declspec(property(get=getStartType, put=putStartType)) ServiceStartType StartType;
		ServiceStartType getStartType(void) const { return m_starttype; }
		void putStartType(ServiceStartType value) { m_starttype = value; }

	private:

		// m_displayname
		//
		// Service display name
		tstring	m_displayname;

		// m_starttype
		//
		// Service start type
		ServiceStartType m_starttype;

		// m_errorcontrol
		//
		// Service error control flag
		ServiceErrorControl m_errorcontrol;
	};

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

	// svctl::service_status_manager
	//
	// Manages the service status. A callback must be provided during construction to implement
	// how the status is ultimately reported, this allows for services running under the local
	// dispatcher to report status as they normally would.
	//
	// Pending statuses (SERVICE_START_PENDING et al) are handled with an automatic checkpoint
	// mechanism that will periodically report progress until a non-pending status is reported
	//
	// Until the service has reached an initial SERVICE_RUNNING state, the accepted controls
	// mask will be zeroed out so that a service will not need to deal with any race conditions
	// during initialization.  Once that initial SERVICE_RUNNING state has been reached, however,
	// the only controls that will be blocked are STOP, PAUSE and CONTINUE during pending state
	// changes.  The service will have to be able to handle any other controls it's registered
	// for or explicitly change them as necessary using the methods in the service class.
	class service_status_manager
	{
	public:

		// report_status_t
		//
		// Function that must be provided during construction to handle the reporting of a 
		// new status to either the service control manager or the local dispatcher
		typedef std::function<void(SERVICE_STATUS_HANDLE, SERVICE_STATUS)> report_status_t;

		// Instance constructor
		//
		explicit service_status_manager(SERVICE_STATUS_HANDLE handle, report_status_t func) : 
			m_reportfunc(func), m_handle(handle) {}

		// Set
		//
		// Sets a new service status
		void Set(ServiceStatus status);
		void Set(ServiceStatus status, uint32_t win32exitcode);
		void Set(ServiceStatus status, uint32_t win32exitcode, uint32_t svcexitcode);

	private:

		service_status_manager(const service_status_manager&)=delete;
		service_status_manager& operator=(const service_status_manager&)=delete;

		// DeriveAcceptedControlsMask
		//
		// Derives a SERVICE_ACCEPT_XXX mask from a ServiceControl mask
		static uint32_t DeriveAcceptedControlsMask(uint32_t controls);

		// m_acceptmask
		//
		// Mask of controls accepted by the service, derived from m_controlmask
		uint32_t m_acceptmask;

		// m_controlmask
		//
		// Mask of controls handled by the service
		uint32_t m_controlmask;

		// m_handle
		//
		// Handle returned from a sucessful call to RegisterServiceCtrlHandler(Ex)
		const SERVICE_STATUS_HANDLE m_handle;

		// m_reportfunc
		//
		// Function pointer used to report an updated service status
		const report_status_t m_reportfunc;
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

	// svctl::winexception
	//
	// specialization of std::exception for Win32 error codes
	class winexception : public std::exception
	{
	public:
		
		// Instance Constructors
		//
		explicit winexception(DWORD result);
		explicit winexception(HRESULT hresult) : winexception(static_cast<DWORD>(hresult)) {}
		winexception() : winexception(GetLastError()) {}

		// Destructor
		virtual ~winexception()=default;

		// std::exception::what
		//
		// 
		virtual const char* what() const { return m_what.c_str(); }

	private:

		// m_what
		//
		// Exception message string derived from the Win32 error code
		std::string m_what;
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

private:

	Service(const Service&)=delete;
	Service& operator=(const Service&)=delete;

	// getServiceName
	//
	// Gets the service name
	static const svctl::tchar_t* getServiceName() { return _T("TODOCHANGEME"); }

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
// ServiceControl
//
// Stronly typed enumeration of SERVICE_CONTROL_XXXX constants

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

//-----------------------------------------------------------------------------
// ServiceErrorControl
//
// Strongly typed enumeration of SERVICE_ERROR constants

enum class ServiceErrorControl
{
	Ignore			= SERVICE_ERROR_IGNORE,
	Normal			= SERVICE_ERROR_NORMAL,
	Severe			= SERVICE_ERROR_SEVERE,
	Critical		= SERVICE_ERROR_CRITICAL,
};

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
// ServiceProcessType
//
// Strongly typed enumeration of service process type flags

enum class ServiceProcessType
{
	Unique			= SERVICE_WIN32_OWN_PROCESS,
	Shared			= SERVICE_WIN32_SHARE_PROCESS,
	Interactive		= SERVICE_INTERACTIVE_PROCESS,
};

//-----------------------------------------------------------------------------
// ServiceStartType
//
// Strongy typed enumeration of SERVICE_XXX_START constants

enum class ServiceStartType
{
	Automatic		= SERVICE_AUTO_START,
	Manual			= SERVICE_DEMAND_START,
	Disabled		= SERVICE_DISABLED,
};

//-----------------------------------------------------------------------------
// ServiceStatus
//
// Strongly typed enumeration of SERVICE_XXXX status constants

enum class ServiceStatus
{
	Stopped				= SERVICE_STOPPED,
	StartPending		= SERVICE_START_PENDING,
	StopPending			= SERVICE_STOP_PENDING,
	Running				= SERVICE_RUNNING,
	ContinuePending		= SERVICE_CONTINUE_PENDING,
	PausePending		= SERVICE_PAUSE_PENDING,
	Paused				= SERVICE_PAUSED,
};

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __SERVICELIB_H_
