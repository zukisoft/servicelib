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
#include <initializer_list>
#include <memory>
#include <vector>

// Windows API
#include <Windows.h>
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

#pragma warning(push, 4)

//-----------------------------------------------------------------------------

namespace svctl {

	typedef char	char_t;
#ifndef _UNICODE
	typedef char	tchar_t;
#else
	typedef wchar_t	tchar_t;
#endif

	// svctl::service_table_entry_t
	//
	// Creates a SERVICE_TABLE_ENTRY for a service class
	template<class _service>
	struct service_table_entry_t : public SERVICE_TABLE_ENTRY {

		// TODO: put a static_assert() here to check the _service argument for the necessary methods
		service_table_entry_t() : lpServiceName(_service::getServiceName()), lpServiceProc(_service::ServiceMain) {}
	};

	// svctl::service_entry
	//
	// Base class used in conjunction with service_entry to provide the static
	// service data and function pointer mappings
	class service_entry
	{
	public:

		service_entry()=default;
		virtual ~service_entry()=default;

		service_entry(const service_entry&)=default;
		service_entry& operator=(const service_entry&)=default;

		// Copies the contained SERVICE_TABLE_ENTRY for use with the
		// StartServiceCtrlDispatcher()
		//
		operator SERVICE_TABLE_ENTRY() const { return m_table; }

	protected:

		SERVICE_TABLE_ENTRY			m_table;		// Service table entry
		LPSERVICE_MAIN_FUNCTION		m_main;			// Non-service Main()

	//private:

	};

	// svctl::service
	//
	// Base class for all standard service templates
	class service
	{
	public:

		virtual ~service()=default;

		// Run
		//
		// Main service loop
		virtual uint32_t Run(uint32_t argc, tchar_t** argv) = 0;

		void Main(uint32_t argc, tchar_t** argv);
		void ServiceMain(uint32_t argc, tchar_t** argv);

	protected:

		service()=default;

	private:

		service(const service&)=delete;
		service& operator=(const service&)=delete;
	};

} // namespace svctl

//
// GLOBAL NAMESPACE CLASSES
//

// ServiceEntry
//
// Creates a service entry for a specific service.  This entry holds pointers
// to all of the static data and functions that can be accessed by the module
template<class _service>
class ServiceEntry : public svctl::service_entry, public _service
{
public:
	// TODO: put a static_assert here to check the _service template argument

	// Instance Constructor
	//
	ServiceEntry()
	{
		// Initialize the SERVICE_TABLE_ENTRY fields
		m_table.lpServiceName = const_cast<svctl::tchar_t*>(_service::getServiceName());
		m_table.lpServiceProc = _service::StaticServiceMain;

		// Initialize remaining static data and function pointers
		m_main = _service::StaticMain;
	}

	// Destructor
	//
	virtual ~ServiceEntry()=default;

	ServiceEntry(const ServiceEntry&)=default;
	ServiceEntry& operator=(const ServiceEntry&)=default;
};

// ServiceModule
//
class ServiceModule
{
public:

	// Instance Constructor
	//
	ServiceModule(const std::initializer_list<svctl::service_entry>& services) 
		: m_services(services) {}

	//-------------------------------------------------------------------------
	// Member Functions

	// main
	//
	// Entry point for console application modules
	int main(int argc, svctl::tchar_t** argv);

	// WinMain
	//
	// Entry point for Windows application modules
	int WinMain(HINSTANCE instance, HINSTANCE previous, svctl::tchar_t* cmdline, int show);

private:

	ServiceModule(const ServiceModule&)=delete;
	ServiceModule& operator=(const ServiceModule&)=delete;

	//-------------------------------------------------------------------------
	// Private Member Functions

	static unsigned __stdcall LocalServiceThread(void* arg);

	// StartLocalServiceDispatcher
	//
	// Emulates StartServiceCtrlDispatcher when run as a normal process
	int StartLocalServiceDispatcher(LPSERVICE_TABLE_ENTRY servicetable);

	//-------------------------------------------------------------------------
	// Member Variables

	std::vector<svctl::service_entry>	m_services;
};

// Service
//
template<class _derived>
class Service : public svctl::service
{
public:

	Service()=default;
	virtual ~Service()=default;

	//virtual void Main(uint32_t argc, svctl::tchar_t** argv) 
	//{
	//	int x = argc;
	//}
	//virtual void ServiceMain(uint32_t argc, svctl::tchar_t** argv) 
	//{
	//	int x = argc;
	//}

	static const svctl::tchar_t* getServiceName(void) { return _T("Hello"); }
	static void WINAPI StaticMain(DWORD argc, LPTSTR* argv);
	static void WINAPI StaticServiceMain(DWORD argc, LPTSTR* argv);

private:

	Service(const Service&)=delete;
	Service& operator=(const Service&)=delete;
};

// Service<_derived>::StaticMain
//
// Static entry point for the service when run as a normal process
template<class _derived>
void Service<_derived>::StaticMain(DWORD argc, LPTSTR* argv)
{
	std::unique_ptr<svctl::service> instance = std::make_unique<_derived>();
	instance->Main(argc, argv);
}

// Service<_derived>::StaticServiceMain
//
// Static entry point for the service when run as a service process
template<class _derived>
void Service<_derived>::StaticServiceMain(DWORD argc, LPTSTR* argv)
{
	std::unique_ptr<svctl::service> instance = std::make_unique<_derived>();
	instance->ServiceMain(argc, argv);
}

//-----------------------------------------------------------------------------

#pragma warning(pop)

#endif	// __SERVICELIB_H_
