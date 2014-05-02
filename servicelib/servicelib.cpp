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

void service::Main(uint32_t argc, tchar_t** argv)
{
	int x = 123;
}

void service::ServiceMain(uint32_t argc, tchar_t** argv)
{
	int x = 123;
}

END_NAMESPACE(svctl)

//-----------------------------------------------------------------------------
// ServiceModule implementation
//-----------------------------------------------------------------------------

// ServiceModule::LocalServiceThread (private, static)
//
// Entry point for a local service dispatcher thread, this is used when the
// module is executed as a normal application rather than a service
unsigned ServiceModule::LocalServiceThread(void* arg)
{
	_ASSERTE(arg != nullptr);

	// Cast the pointer back out into an LPSERVICE_MAIN_FUNCTION pointer and invoke
	LPSERVICE_MAIN_FUNCTION func = reinterpret_cast<LPSERVICE_MAIN_FUNCTION>(arg);
	func(0, nullptr);

	// TODO: Need a way to provide arguments to func() !!!!
	// TODO: This is always going to call ServiceMain() rather than Main()
	//    switch it up in the table passed to the local dispatcher

	return 0;
}

// ServiceModule::main
//
// Entry point for console-based applications
int ServiceModule::main(int argc, svctl::tchar_t** argv)
{
	std::vector<SERVICE_TABLE_ENTRY> servicetable;		// Services to dispatch

	// PROCESS COMMAND LINE ARGUMENTS
	// PERFORM INITIALIZATIONS

	// Create the SERVICE_TABLE_ENTRY array for StartServiceCtrlDispatcher
	// TODO: This needs to be filtered
	for(auto iterator : m_services) { servicetable.push_back(iterator); }
	servicetable.push_back( { nullptr, nullptr } );

	// Attempt to start the service control dispatcher
	if(StartServiceCtrlDispatcher(servicetable.data())) return 0;

	// ERROR_FAILED_SERVICE_CONTROLLER_CONNECT is indicated when the module
	// is being executed as a normal application rather than a service
	DWORD result = GetLastError();
	return (result == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) ?
		StartLocalServiceDispatcher(servicetable.data()) : static_cast<int>(result);
}

// ServiceModule::StartLocalServiceDispatcher (private)
//
// Emulated service dispatcher when module is executed as a normal application
int ServiceModule::StartLocalServiceDispatcher(LPSERVICE_TABLE_ENTRY servicetable)
{
	std::vector<HANDLE>			threads;		// Worker thread handles
	int							result;			// Result from function call

	// Create a worker thread for each service in the service table
	for(LPSERVICE_TABLE_ENTRY entry = servicetable; entry->lpServiceName; entry++) {

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

	// Wait for all the worker threads to terminate
	WaitForMultipleObjects(threads.size(), threads.data(), TRUE, INFINITE);

	// Handles created with _beginthreadex() are not closed automatically when the thread ends
	for(auto iterator : threads) CloseHandle(iterator);
	return 0;
}

// ServiceModule::WinMain
//
// Entry point for Windows-based applications
int ServiceModule::WinMain(HINSTANCE instance, HINSTANCE previous, svctl::tchar_t* cmdline, int show)
{
	UNREFERENCED_PARAMETER(instance);
	UNREFERENCED_PARAMETER(previous);
	UNREFERENCED_PARAMETER(cmdline);
	UNREFERENCED_PARAMETER(show);

	// Currently not doing anything with the WinMain arguments, and Visual C++
	// provides the argc/argv arguments for main() as part of the runtime library
	return main(__argc, __targv);
}

//-----------------------------------------------------------------------------

#pragma pop_macro("END_NAMESPACE")
#pragma pop_macro("BEGIN_NAMESPACE")
#pragma warning(pop)
