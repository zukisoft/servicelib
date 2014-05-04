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

// BEGIN_NAMESPACE / END_NAMESPACE
//
#pragma push_macro("BEGIN_NAMESPACE")
#pragma push_macro("END_NAMESPACE")
#undef BEGIN_NAMESPACE
#undef END_NAMESPACE
#define BEGIN_NAMESPACE(ns)		namespace ns {
#define END_NAMESPACE(ns)		}

//-----------------------------------------------------------------------------

BEGIN_NAMESPACE(svctl)

//-----------------------------------------------------------------------------
// svctl::auxiliary_state_machine
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// auxiliary_state_machine::Initialize
//
// Invoked to initialize all registered virtual auxiliary base classes
//
// Arguments:
//
//	servicename		- Name associated with the derived service

void auxiliary_state_machine::Initialize(const tchar_t* servicename)
{
	_ASSERTE(servicename != nullptr);
	if(!servicename) throw winexception(E_INVALIDARG);

	// Invoke auxiliary_state::OnInitialize for all registered instances
	for(const auto& iterator : m_instances) iterator->OnInitialize(servicename);
}

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
// service
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// service::ControlRequest (private)
//
// Initiates a service control request
//
// Arguments:
//
//	control			- Service control code
//	type			- Control-specific event type
//	data			- Control-specific event data

uint32_t service::ControlRequest(uint32_t control, uint32_t type, void* data)
{
	return NO_ERROR;
}

//-----------------------------------------------------------------------------
// service::ControlRequestCallback (private, static)
//
// Control handler callback registered with the service control manager
//
// Arguments:
//
//	control			- Service control code
//	type			- Control-specific event type
//	data			- Control-specific event data
//	context			- Context pointer passed into RegisterServiceCtrlHandlerEx()

DWORD WINAPI service::ControlRequestCallback(DWORD control, DWORD type, void* data, void* context)
{
	_ASSERTE(context != nullptr);
	return static_cast<DWORD>(reinterpret_cast<service*>(context)->ControlRequest(control, type, data));
}

//-----------------------------------------------------------------------------
// service::LocalMain
//
// Service entry point when process is being run as a normal application
//
// Arguments:
//
//	argc		- Number of command line arguments
//	argv		- Array of command-line argument strings

void service::LocalMain(uint32_t argc, tchar_t** argv)
{
	m_statushandle = 0;
	auxiliary_state_machine::Initialize(GetServiceName());
}

//-----------------------------------------------------------------------------
// service::ServiceMain
//
// Service entry point when process is being run as a service
//
// Arguments:
//
//	argc		- Number of command line arguments
//	argv		- Array of command-line argument strings

void service::ServiceMain(uint32_t argc, tchar_t** argv)
{
	// Register the control handler callback for this service
	m_statushandle = RegisterServiceCtrlHandlerEx(GetServiceName(), ControlRequestCallback, this);
	if(m_statushandle == 0) {

		int x = 123;
	}

	// SET SERVICE_START_PENDING
	auxiliary_state_machine::Initialize(argv[0]);

	// OnStart(argc, argv)
	// SERVICE_RUNNING

	// Run
	// SERVICE_STOP_PENDING

	// state_machine::Terminate
	// SERVICE_STOPPED

	//////////////
	// Once RegisterServiceCtrlHandlerEx has been called successfully, we can
	// report error codes via the service control manager status
	//////////////
}

//
// >> svctl::service_module
//

// svctl::service_module::Dispatch
//
// Dispatches a filtered collection of services to either the service control
// manager or the local dispatcher depending on the runtime context
//
int service_module::Dispatch(const std::vector<service_table_entry>& services)
{
	std::vector<SERVICE_TABLE_ENTRY>	table;			// Table of services to dispatch
	int									result = 0;		// Result from function call

	// Create the SERVICE_TABLE_ENTRY array for StartServiceCtrlDispatcher
	for(const auto& iterator : services) table.push_back(iterator.ServiceEntry);
	table.push_back( { nullptr, nullptr } );

	// Attempt to start the service control dispatcher
	if(!StartServiceCtrlDispatcher(table.data())) {
	
		result = static_cast<int>(GetLastError());		// Get the result code

		// ERROR_FAILED_SERVICE_CONTROLLER_CONNECT is indicated when the module is being
		// executed in a normal runtime context as opposed to a service runtime context
		if(result == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
	
			// Clear and reinitialize the SERVICE_TABLE_ENTRY array using the local entries
			// and invoke the local service dispatcher instead
			table.clear();
			for(const auto& iterator : services) table.push_back(iterator.LocalServiceEntry);
			table.push_back( { nullptr, nullptr } );
			result = StartLocalServiceDispatcher(table.data());
		}
	}

	return result;
}

// svctl::service_module::LocalServiceThread (private, static)
//
// Entry point for a local service dispatcher thread
//
unsigned service_module::LocalServiceThread(void* arg)
{
	_ASSERTE(arg != nullptr);

	// The argument passed into the worker thread is the SERVICE_TABLE_ENTRY
	LPSERVICE_TABLE_ENTRY entry = reinterpret_cast<LPSERVICE_TABLE_ENTRY>(arg);

	// TODO: Don't need this anymore, arg should instead be used for the entry
	// as well a full command line argument set to facilitate that functionality

	// Local dispatcher doesn't support start arguments, but the service name
	// must be set as argument zero for compatibility
	std::array<tchar_t*, 2> argv = { entry->lpServiceName, nullptr };
	entry->lpServiceProc(1, argv.data());

	return 0;
}

// svctl::service_module::ModuleMain
//
// Implements the entry point for the service module
//
int service_module::ModuleMain(int argc, svctl::tchar_t** argv)
{
	// PROCESS COMMAND LINE ARGUMENTS TO FILTER THE LIST
	// run
	// install
	// uninstall
	// regserver
	// unregserver
	// and so on

	std::vector<service_table_entry> filtered;
	for(const auto& iterator : m_services) filtered.push_back(iterator);

	// Dispatch the filtered list of service classes
	return Dispatch(filtered);
}

// svctl::service_module::StartLocalServiceDispatcher (private)
//
// Emulates the StartServiceCtrlDispatcher system function to execute a collection
// of services in a normal user-mode application context
//
int service_module::StartLocalServiceDispatcher(LPSERVICE_TABLE_ENTRY table)
{
	std::vector<HANDLE>			threads;		// Worker thread handles
	int							result;			// Result from function call

	// Create a suspended worker thread for each service in the service table
	for(LPSERVICE_TABLE_ENTRY entry = table; entry->lpServiceName; entry++) {

		// Attempt to create a suspended worker thread for the service
		uintptr_t thread = _beginthreadex(nullptr, 0, LocalServiceThread, entry, CREATE_SUSPENDED, nullptr);
		if(thread == 0) {

			// Unable to create one of the worker threads, close out the ones that have been
			// successfully created and bail (should probably convert errno to Win32 here)
			_get_errno(&result);
			for(const auto& iterator : threads) CloseHandle(iterator);
			return result;
		}

		// Keep track of the newly created thread as a normal Win32 HANDLE
		threads.push_back(reinterpret_cast<HANDLE>(thread));
	}

	// Start all the worker threads
	for(const auto& iterator : threads) ResumeThread(iterator);

	//
	// TODO: The idea here is to have a command processor that allows the user
	// to control the services as if they were the SCM.  For example ....
	//
	// start myservice [args]
	// pause myservice
	// list services
	// etc.
	//
	// when the last service stops, the process exits just like it would normally
	// trace messages (when I get there) would display in the console
	//
	// this is dummy code:
	HWND console = GetConsoleWindow();
	bool freeconsole = false;
	if(console == nullptr) freeconsole = AllocConsole() ? true : false;

	// end dummy code

	// Wait for all the worker threads to terminate and closet their handles
	WaitForMultipleObjects(threads.size(), threads.data(), TRUE, INFINITE);
	for(const auto& iterator : threads) CloseHandle(iterator);

	// dummy
	if(freeconsole) FreeConsole();
	// end dummy

	return 0;
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

winexception::winexception(DWORD result)
{
	LPSTR				formatted;				// Formatted message

	// Invoke FormatMessageA to convert the system error code into an ANSI string; use a lame
	// generic 'unknown' string for any codes that cannot be looked up successfully
	if(FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, result,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&formatted), 0, nullptr)) {
		
		m_what = formatted;						// Store the formatted message string
		LocalFree(formatted);					// Release FormatMessage() allocated buffer
	}

	else m_what = "Unknown Windows status code " + std::to_string(result);
}

//-----------------------------------------------------------------------------

END_NAMESPACE(svctl)

//-----------------------------------------------------------------------------

#pragma pop_macro("END_NAMESPACE")
#pragma pop_macro("BEGIN_NAMESPACE")
#pragma warning(pop)
