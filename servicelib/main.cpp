// servicelib.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "servicelib.h"
#include "resource.h"


// MinimalService Sample
//
class MinimalService : public Service<MinimalService>
{
public:

	// Constructor / Destructor
	MinimalService()=default;
	virtual ~MinimalService()=default;

private:

	MinimalService(const MinimalService&)=delete;
	MinimalService& operator=(const MinimalService&)=delete;

	// CONTROL_HANDLER_MAP
	//
	// Series of ATL-like macros that define a mapping of service
	// control codes to handler functions
	BEGIN_CONTROL_HANDLER_MAP(MinimalService)
		CONTROL_HANDLER_ENTRY(ServiceControl::Stop, OnStop)
	END_CONTROL_HANDLER_MAP()

	// OnStart (Service)
	//
	// Every service that derives from Service<> must define an OnStart()
	void OnStart(int argc, LPTSTR* argv)
	{
		UNREFERENCED_PARAMETER(argc);
		UNREFERENCED_PARAMETER(argv);
	}

	// OnStop
	//
	// Sample control handler for ServiceControl::Stop
	void OnStop(void)
	{
	}
};

class MyService : public Service<MyService>
{
public:

	MyService()=default;

	void OnStart(int argc, svctl::tchar_t** argv)
	{
		UNREFERENCED_PARAMETER(argc);
		UNREFERENCED_PARAMETER(argv);

		std::vector<svctl::tstring> multisz = m_multitest;
		// wow, that actually worked ^^^^^^^^

		//throw ServiceException(5L);

		//Sleep(5000);

		//std::async(std::launch::async, [=]() { 
		//
		//	while(true) {
		//		svctl::tstring param = m_paramtestsz;
		//		OutputDebugString(param.c_str());
		//		OutputDebugString(_T("\r\n"));
		//		Sleep(100);
		//	}
		//});

	}

	void OnStop(void)
	{
		//Sleep(5000);
		int x = 123;
	}

	BEGIN_CONTROL_HANDLER_MAP(MyService)
		CONTROL_HANDLER_ENTRY(ServiceControl::Stop, OnStop)
	END_CONTROL_HANDLER_MAP()

	BEGIN_PARAMETER_MAP(MyService)
		PARAMETER_ENTRY(_T("TestSz"), m_paramtestsz)
		PARAMETER_ENTRY(_T("MyStringRegSz"), m_multitest);
	END_PARAMETER_MAP()

private:

	MyService(const MyService&)=delete;
	MyService& operator=(const MyService&)=delete;

	StringParameter m_paramtestsz { _T("defaultparam") };
	MultiStringParameter m_multitest;
};

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{

#ifdef _DEBUG

	int nDbgFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);	// Get current flags
	nDbgFlags |= _CRTDBG_LEAK_CHECK_DF;						// Enable leak-check
	_CrtSetDbgFlag(nDbgFlags);								// Set the new flags

#endif	// _DEBUG

	ServiceHarness<MyService> runner;
	runner.SetParameter(L"parameter1", 0x123456);
	runner.SetParameter(L"parameter2", 0x1);
	runner.SetParameter(L"parameter3", 0x123456789);
	runner.SetParameter(L"MyStringRegSz", { _T("MyValueSz"), _T("MyValueSz2") } );

	runner.Start(IDS_MYSERVICE, 1, 1.0, true, svctl::tstring(L"sweet"), 14, L"last");
	if(runner.CanStop) runner.Stop();

	//runner.Start(IDS_MYSERVICE);
	//runner.Stop();

	return 0;

	// Manual dispatching with dynamic names
	ServiceTable services = { ServiceTableEntry<MyService>(_T("MyService")), ServiceTableEntry<MinimalService>(IDS_MYSERVICE) };
	services.Dispatch();
}

