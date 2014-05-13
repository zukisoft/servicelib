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

	// Create the SERVICE_TABLE_ENTRY array for StartServiceCtrlDispatcher
	for(const auto& iterator : services) table.push_back(iterator.ServiceEntry);
	table.push_back( { nullptr, nullptr } );

	// Attempt to start the service control dispatcher
	if(!StartServiceCtrlDispatcher(table.data())) return static_cast<int>(GetLastError());

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

END_NAMESPACE(svctl)

//-----------------------------------------------------------------------------

#pragma pop_macro("END_NAMESPACE")
#pragma pop_macro("BEGIN_NAMESPACE")
#pragma warning(pop)
