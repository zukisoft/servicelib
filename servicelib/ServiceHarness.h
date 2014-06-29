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

#ifndef __SERVICEHARNESS_H_
#define __SERVICEHARNESS_H_
#pragma once

#include "Service.h"

#pragma warning(push, 4)

//-----------------------------------------------------------------------------
// SERVICE TEMPLATE LIBRARY INTERNALS
//
// Namespace: svctl
//-----------------------------------------------------------------------------

namespace svctl {

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

		// SetParameter (ServiceParameterFormat::Binary)
		//
		// Sets a binary parameter key/value pair
		template <typename _type> typename std::enable_if<!std::is_integral<_type>::value && std::is_trivial<_type>::value, void>::type
		SetParameter(LPCTSTR name, const _type& value) { SetParameter(name, ServiceParameterFormat::Binary, &value, sizeof(_type)); }

		template <typename _type> typename std::enable_if<!std::is_integral<_type>::value && std::is_trivial<_type>::value, void>::type
		SetParameter(unsigned int name, const _type& value) { SetParameter(resstring(name).c_str(), ServiceParameterFormat::Binary, &value, sizeof(_type)); }

		// SetParameter (ServiceParameterFormat::DWord)
		//
		// Sets a 32-bit integer parameter key/value pair
		template <typename _type> typename std::enable_if<std::is_integral<_type>::value && (sizeof(_type) < sizeof(uint64_t)), void>::type
		SetParameter(LPCTSTR name, const _type& value) { SetParameter(name, ServiceParameterFormat::DWord, &value, sizeof(_type)); }

		template <typename _type> typename std::enable_if<std::is_integral<_type>::value && (sizeof(_type) < sizeof(uint64_t)), void>::type
		SetParameter(unsigned int name, const _type& value) { SetParameter(resstring(name).c_str(), ServiceParameterFormat::DWord, &value, sizeof(_type)); }

		// SetParameter (ServiceParameterFormat::MultiString)
		//
		// Sets a string array parameter key/value pair
		void SetParameter(LPCTSTR name, std::initializer_list<const tchar_t*> value);
		void SetParameter(unsigned int name, std::initializer_list<const tchar_t*> value) { SetParameter(resstring(name).c_str(), value); }
		// TODO: forward iterator overload

		// SetParameter (ServiceParameterFormat::QWord)
		//
		// Sets a 64-bit integer parameter key/value pair
		template <typename _type> typename std::enable_if<std::is_integral<_type>::value && (sizeof(_type) == sizeof(uint64_t)), void>::type
		SetParameter(LPCTSTR name, const _type& value) { SetParameter(name, ServiceParameterFormat::QWord, &value, sizeof(_type)); }

		template <typename _type> typename std::enable_if<std::is_integral<_type>::value && (sizeof(_type) == sizeof(uint64_t)), void>::type
		SetParameter(unsigned int name, const _type& value) { SetParameter(resstring(name).c_str(), ServiceParameterFormat::QWord, &value, sizeof(_type)); }

		// SetParameter (ServiceParameterFormat::String)
		//
		// Sets a string parameter key/value pair
		void SetParameter(LPCTSTR name, const tchar_t* value);
		void SetParameter(unsigned int name, const tchar_t* value) { SetParameter(resstring(name).c_str(), value); }

		void SetParameter(LPCTSTR name, const tstring& value);
		void SetParameter(unsigned int name, const tstring& value) { SetParameter(resstring(name).c_str(), value); }

		// Start
		//
		// Starts the service, optionally specifying a variadic set of command line arguments.
		// (Arguments can be C-style strings, fundamental data types, or tstring references)
		template <typename... _arguments>
		void Start(const resstring& servicename, const _arguments&... arguments)
		{
			// Construct a vector<> for the arguments starting with the service name
			// and recursively invoke one of the variadic overloads until done
			std::vector<tstring> argvector { servicename };
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

		// parameter_compare
		//
		// Case-insensitive key comparison for the parameter collection
		struct parameter_compare 
		{ 
			bool operator() (const tstring& lhs, const tstring& rhs) const 
			{
				return _tcsicmp(lhs.c_str(), rhs.c_str()) < 0;
			}
		};

		// parameter_value
		//
		// Parameter collection value type
		using parameter_value = std::pair<ServiceParameterFormat, std::vector<uint8_t>>;

		// parameter_collection
		//
		// Collection used to hold instance-specific parameters
		using parameter_collection = std::map<tstring, parameter_value, parameter_compare>;

		// CloseParameterStoreFunc
		//
		// Function invoked by the service to close parameter storage
		void CloseParameterStoreFunc(void* handle);

		// LoadParameterFunc
		//
		// Function invoked by the service to load a parameter value
		size_t LoadParameterFunc(void* handle, const tchar_t* name, ServiceParameterFormat format, void* buffer, size_t length);

		// OpenParameterStoreFunc
		//
		// Function invoked by the service to open parameter storage
		void* OpenParameterStoreFunc(const tchar_t* servicename);

		// RegisterHandlerFunc
		//
		// Function invoked by the service to register it's control handler
		SERVICE_STATUS_HANDLE RegisterHandlerFunc(LPCTSTR servicename, LPHANDLER_FUNCTION_EX handler, LPVOID context);

		// ServiceControlAccepted (static)
		//
		// Checks a ServiceControl against a SERVICE_ACCEPTS_XXXX mask
		static bool ServiceControlAccepted(ServiceControl control, DWORD mask);

		// SetParameter
		//
		// Internal version of SetParameter, accepts the type and raw parameter data
		void SetParameter(const tchar_t* name, ServiceParameterFormat format, const void* value, size_t length);
		void SetParameter(const tchar_t* name, ServiceParameterFormat format, std::vector<uint8_t>&& value);

		// SetStatusFunc
		//
		// Function invoked by the service to report a status change
		BOOL SetStatusFunc(SERVICE_STATUS_HANDLE handle, LPSERVICE_STATUS status);

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

		// m_parameters
		//
		// Service parameter storage collection
		parameter_collection m_parameters;

		// m_paramlock
		//
		// Parameter collection synchronization object
		std::recursive_mutex m_paramlock;

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

#pragma warning(pop)

#endif	// __SERVICEHARNESS_H_
