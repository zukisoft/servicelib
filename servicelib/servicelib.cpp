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
// svctl::service implementation
//-----------------------------------------------------------------------------

BEGIN_NAMESPACE(svctl)

//
// >> svctl::resstring
//

// svctl::resstring::GetResourceString (private, static)
//
// Gets a read-only pointer to a module string resource
//
const tchar_t* resstring::GetResourceString(unsigned int id, HINSTANCE instance)
{
	static const tchar_t EMPTY[] = _T("");		// Used for missing resources

	// LoadString() has a neat trick to return a read-only string pointer
	tchar_t* string = nullptr;
	int result = LoadString(instance, id, reinterpret_cast<tchar_t*>(&string), 0);

	// Return an empty string rather than a null pointer if the resource was missing
	return (result == 0) ? EMPTY : string;
}
	
//
// >> svctl::service
//

void service::LocalMain(uint32_t argc, tchar_t** argv)
{
	int x = 123;
}

void service::ServiceMain(uint32_t argc, tchar_t** argv)
{
	int x = 123;
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

	//
	// DEFECT: These iterators are temporary and disappear along with the name strings
	//

	// Create the SERVICE_TABLE_ENTRY array for StartServiceCtrlDispatcher
	for(auto iterator : services) table.push_back(iterator.ServiceEntry);
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
			for(auto iterator : services) table.push_back(iterator.LocalServiceEntry);

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

	// Cast the pointer back out into an LPSERVICE_MAIN_FUNCTION pointer and invoke
	reinterpret_cast<LPSERVICE_MAIN_FUNCTION>(arg)(0, nullptr);

	return 0;
}

// svctl::service_module::ModuleMain
//
// Implements the entry point for the service module
//
int service_module::ModuleMain(int argc, svctl::tchar_t** argv)
{
	// PROCESS COMMAND LINE ARGUMENTS TO FILTER THE LIST

	std::vector<service_table_entry> filtered;
	for(auto iterator : m_services) filtered.push_back(iterator);

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
		uintptr_t thread = _beginthreadex(nullptr, 0, LocalServiceThread, entry->lpServiceProc, CREATE_SUSPENDED, nullptr);
		if(thread == 0) {

			// Unable to create one of the worker threads, close out the ones that have been
			// successfully created and bail (should probably convert errno to Win32 here)
			_get_errno(&result);
			for(auto iterator : threads) CloseHandle(iterator);
			return result;
		}

		// Keep track of the newly created thread as a normal Win32 HANDLE
		threads.push_back(reinterpret_cast<HANDLE>(thread));
	}

	// Start all the worker threads
	for(auto iterator : threads) ResumeThread(iterator);

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
	for(auto iterator : threads) CloseHandle(iterator);

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
