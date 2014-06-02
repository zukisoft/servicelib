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
		CONTROL_HANDLER(ServiceControl::Stop, OnStop)
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

	virtual void IterateParameters(std::function<void(const svctl::tstring& name, svctl::parameter& param)> func)
	{
		func(svctl::resstring(IDS_STRING101), m_test1);
		func(L"Test2", m_test2);
	}

	void BindParameters(void)
	{
		IterateParameters([&](const svctl::tstring& name, const svctl::parameter& param) -> void {

			const svctl::parameter* p = &param;
			int x = 123;
		});
	}

	ServiceParameter<int> m_test1 = 123;
	ServiceParameter<DWORD> m_test2;

	void OnStart(int argc, svctl::tchar_t** argv)
	{
		UNREFERENCED_PARAMETER(argc);
		UNREFERENCED_PARAMETER(argv);

		//state_machine::DoStuff();

		//wchar_t temp[255];
		//wsprintf(temp, L"MyService (0x%08X)::OnStart  argc=%d, argv[0] = %s\r\n", this, argc, argv[0]);
		//OutputDebugString(temp);
	}

	void OnStop(void)
	{
	}

	//void OnPause(void)
	//{
	//	//Sleep(15000);
	//}

	DWORD OnMyCommand(void)
	{
		OutputDebugString(L"MyService::OnMyCommand!\r\n");
		return ERROR_SUCCESS;
	}

	BEGIN_CONTROL_HANDLER_MAP(MyService)
		CONTROL_HANDLER(ServiceControl::Stop, OnStop)
		///CONTROL_HANDLER(ServiceControl::Pause, OnPause)
		///CONTROL_HANDLER(200, OnMyCommand)
	END_CONTROL_HANDLER_MAP()

private:

	MyService(const MyService&)=delete;
	MyService& operator=(const MyService&)=delete;
};

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{

#ifdef _DEBUG

	int nDbgFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);	// Get current flags
	nDbgFlags |= _CRTDBG_LEAK_CHECK_DF;						// Enable leak-check
	_CrtSetDbgFlag(nDbgFlags);								// Set the new flags

#endif	// _DEBUG

	MyService svc;
	svc.BindParameters();

	MyService svc2;
	svc2.BindParameters();

	return 0;

	// Manual dispatching with dynamic names
	ServiceTable services = { ServiceTableEntry<MyService>(L"MyService"), ServiceTableEntry<MinimalService>(IDS_MYSERVICE) };
	services.Dispatch();
}

