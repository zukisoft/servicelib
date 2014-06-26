
SERVICE TEMPLATE LIBRARY
Copyright (c) 2001-2014 Michael G. Brehm
 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
 
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

>> GENERAL DOCUMENTATION HERE

>> BRIEF HISTORY COMPARISON WITH OLD SVCTL (WHAT HAS BEEN REMOVED)

- only going to cook in ServiceParameters, old aux classes gone
- async handlers and event object handlers are gone
- automatic status
- etc.

>> SAMPLE SERVICES

>> SERVICE TABLES 

>> INSTALL/REMOVE
- with sc
- with code

>> SERVICE MODULES / AUTOMATING SERVICE TABLES AND INSTALL/REMOVE

>> COM SERVICE (need comservice.h / comservice.cpp?)
>> RPC SERVICE (need rpcservice.h / rpcservice.cpp?)

>> HOW TO DEBUG SERVICES

------------------
SERVICE PARAMETERS
------------------

Support for read-only service parameters is provided by the following data types
that can be declared as derived service class member variables.
// todo: can be overridden, default is registry key

DWordParameter (uint32_t)
- Registry type: REG_BINARY or REG_DWORD

MultiStringParameter (std::vector<svctl::tstring>)
- Registry type: REG_MULTI_SZ

QWordParameter (uint64_t)
- Registry type: REG_BINARY or REG_QWORD

StringParameter (svctl::tstring)
- Registry type: REG_SZ or REG_EXPAND_SZ

BinaryParameter<T> (T)
- Registry type: REG_BINARY
- T must be a trivial data type, see std::is_trivial() for a definition of what qualifies

Declare the parameter(s) as member variable(s) in the service class, optionally
specifying a default value.  If no default value is set at construction, the value
will either be set to zero (DWordParameter, QWordParameter, BinaryParameter<T>), an
empty string (StringParameter) or an empty vector<> (MultiStringParameter):

	private:
		...
		StringParameter m_mysz { _T("defaultvalue") };
		DWordParameter m_mydword;

In order for the service to locate and bind each parameter, they must be exposed via
the BEGIN_PARAMETER_MAP() set of macros, similar to how control handlers are exposed.
Static constant parameter value names can be used, as can resource string identifiers
provided that the string table is part of the local executable's resources:

	BEGIN_PARAMETER_MAP(MyService)
		PARAMETER_ENTRY(IDS_MYSZ_NAME, m_mysz)
		PARAMETER_ENTRY(_T("MyDWORDParameter"), m_mydword)
	END_PARAMETER_MAP()

// todo: now can be overridden
At service startup, the PARAMETER_MAP is iterated and each parameter is bound to the 
HLKM\System\CurrentControlSet\Services\{service name}\Parameters registry key.  If this
parent key does not exist, it will be created automatically.  Each parameter will then
be loaded from the parameters key using the name specified in the PARAMETER_MAP.

If any parameters exist in the PARAMETER_MAP, the service will also automatically accept
the SERVICE_CONTROL_PARAMCHANGE control, and will reload each bound parameter in response
to this service control *before* it is passed onto the service class itself.  This makes
it unnecessary for the service to declare a custom handler if no other meaningful work
needs to be done in response to SERVICE_CONTROL_PARAMCHANGE.

Parameter instances are intended to be used as cached read-only values that can be
copied into local variables as needed, they should not be used directly.  A *copy* of the
underlying value are returned each time a parameter is accessed, this behavior should be 
kept in mind, especially for the MultiStringParameter and BinaryParameter<T> versions, as
they potentially store a great deal of data.  This behavior is enforced to allow the parameters
to be thread-safe with the SERVICE_CONTROL_PARAMCHANGE automatic handler.

Correct way to declare and access parameters:

	BEGIN_PARAMETER_MAP(MyService)
		PARAMETER_ENTRY(_T("RetryCount"), m_retrycount)
		PARAMETER_ENTRY(_T("TempDirectory"), m_tempdir)
	END_PARAMETER_MAP()

	DWordParameter m_retrycount = 10;
	StringParameter m_tempdir;

	void OnStart(DWORD argc, LPTSTR* argv)
	{
		svctl::tstring tempdir = m_tempdir;	
		CreateTempDirectoryToHoldInterestingStuff(tempdir);
		...		
		uint32_t retrycount = m_retrycount;
		while(--retrycount) { 
			...
		}
	}

Parameters can be manually reloaded from the registry by invoking the Load method, although
if the parameter was not successfully bound to the service's Parameters registry key, this
method will do nothing.  Detection of a successful load from the registry can be accomplished
by checking the IsDefaulted property of the parameter variable.  This will be set to false once
the value has been loaded from the registry at least once

// todo: can now be overriden in derived service to not use the registry

-------------------------------
SERVICE TEST HARNESS (EXTERNAL)
-------------------------------

Header File: ServiceHarness.h
Implementation File: ServiceHarness.cpp

Services generated with the service template library can also be run under the context of a provided
test harness class, ServiceHarness<>.  This class provides the ability to start, stop and send control
codes to the service class.  Mimicking control codes that require event data is supported, but it's
left up to the harness application to send in something realistic.

Service command line arguments can be provided when starting the service by appending them to the
Start(servicename) method.  This method is variadic and can accept any fundamental data type supported
by std::to_[w]string(), a constant generic text string pointer, or a reference to an std::[w]string object:

	Start(L"MyService", std::wstring(L"argumentone"), 11, 123.45, true, L"lastargument")

The simplest pattern for using ServiceHarness<> requires invoking the Start(servicename) method, waiting
for some trigger to occur that will terminate the harness application, and then invoking Stop():

	...
	ServiceHarness<MyService> harness;
	harness.Start(_T("MyServiceName"));

	// wait for some trigger here to stop the service test harness
	harness.Stop();
	...


>> PARAMETERS DOCUMENTATION GOES HERE
	need note about EXPAND_SZ, will not automatically expand variables like with registry
	
	...
	ServiceHarness<MyService> harness;
	harness.SetParameter(_T("MyStringParameter"), _T("PathToAnInterestingFile"));
	harness.SetParameter(_T("MyDWordParameter"), 0x12345678);
	harness.Start(_T("MyServiceName"));

	// parameters can be set at any time, service does not need to be stopped
	harness.SetParameter(_T("StringArrayAfterStart"), { _T("String1"), _T("String2"), _T("String3") });
	...


ServiceHarness<> Methods:
-------------------------

void Continue(void)
	- Sends ServiceControl::Continue to the service
	- Waits for service to reach ServiceStatus::Running
	- Throws ServiceException& on error or if service stops prematurely

void Pause(void)
	- Sends ServiceControl::Pause to the service
	- Waits for service to reach ServiceStatus::Paused
	- Throws ServiceException& on error or if service stops prematurely

DWORD SendControl(ServiceControl control)
DWORD SendControl(ServiceControl control, DWORD eventtype, LPVOID eventdata)
	- Sends a control code to the service, optionally specifying event information (this is not common)
	- Returns a status code similar to Win32 API's ControlService() method, should not throw an exception

void SetParameter(LPCTSTR name, <type>& value)
	- Sets a local service parameter key/value pair with a format derived from <type>
	- Throws ServiceException& on error

		<type>                               Implied format                       Service<> declaration
		------                               --------------                       ---------------------
		8-bit integer                        ServiceParameterFormat::DWord        DWordParameter
		16-bit integer                       ServiceParameterFormat::DWord        DWordParameter
		32-bit integer                       ServiceParameterFormat::DWord        DWordParameter
		64-bit integer                       ServiceParameterFormat::QWord        QWordParameter
		const TCHAR*                         ServiceParameterFormat::String       StringParameter
		const std::[w]string&                ServiceParameterFormat::String       StringParameter
		std::initializer_list<const TCHAR*>  ServiceParameterFormat::MultiString  MultiStringParameter
		// more MultiString overloads will come here
		[any other trivial data type]        ServiceParameterFormat::Binary       BinaryParameter

void Start(LPCTSTR servicename, ...)
void Start(unsigned int servicename, ...)
	- Starts the service with the specified service name and optional command line arguments
	- unsigned int overload accepts a resource string id for the service name
	- If service does not reach ServiceStatus::StartPending in 30 seconds, will raise an exception
	- Waits for the service to reach ServiceStatus::Running
	- Throws ServiceException& on error or if service stops prematurely

void Stop(void)
	- Sends ServiceControl::Stop to the service
	- Waits for service to reach ServiceStatus::Stopped
	- Throws ServiceException& on error or if service stops prematurely

bool WaitForStatus(ServiceStatus status, uint32_t timeout = INFINITE)
	- Optional timeout value is specified in milliseconds
	- Waits for the service to reach the specified status
	- Returns TRUE if service reached the status, false if the operation timed out
	- Throws ServiceException& on error or if service stops prematurely


ServiceHarness<> Properties:
----------------------------

bool CanContinue (read-only)
	- Determines if the service is capable of accepting ServiceControl::Continue

bool CanPause (read-only)
	- Determines if the service is capable of accepting ServiceControl::Pause

bool CanStop (read-only)
	- Determines if the service is capable of accepting ServiceControl::Stop

SERVICE_STATUS Status (read-only)
	- Gets a copy of the current SERVICE_STATUS structure for the service
