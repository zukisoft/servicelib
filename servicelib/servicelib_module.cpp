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
// svctl::service_module
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// service_module::Dispatch
//
// Dispatches a filtered collection of services to either the service control
// manager or the local dispatcher depending on the runtime context
//
// Arguments:
//
//	service		- Filtered collection of services to be dispatched

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

//-----------------------------------------------------------------------------
// service_module::LocalServiceThread (private, static)
//
// Entry point for a local service dispatcher thread
//
// Arguments:
//
//	arg		- Argument passed to _beginthreadex()

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

//-----------------------------------------------------------------------------
// service_module::ModuleMain
//
// Implements the entry point for the service module
//
// Arguments:
//
//	argc		- Number of command line arguments
//	argv		- Array of command line argument strings

int service_module::ModuleMain(int argc, svctl::tchar_t** argv)
{
	// PROCESS COMMAND LINE ARGUMENTS TO FILTER THE LIST
	// run
	// install
	// uninstall
	// regserver
	// unregserver
	// and so on

	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	std::vector<service_table_entry> filtered;
	for(const auto& iterator : m_services) filtered.push_back(iterator);

	// Dispatch the filtered list of service classes
	return Dispatch(filtered);
}

//-----------------------------------------------------------------------------
// service_module::StartLocalServiceDispatcher (private)
//
// Emulates the StartServiceCtrlDispatcher system function to execute a collection
// of services in a normal user-mode application context
//
// Arguments:
//
//	table		- Table of services to dispatch

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

END_NAMESPACE(svctl)

//-----------------------------------------------------------------------------

#pragma pop_macro("END_NAMESPACE")
#pragma pop_macro("BEGIN_NAMESPACE")
#pragma warning(pop)
