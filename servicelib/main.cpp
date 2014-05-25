// servicelib.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "servicelib.h"
#include "resource.h"
#include <thread>
#include <mutex>
#include <future>

////////////////////////////////////////////////////

class MyService : public Service<MyService>
{
public:

	MyService()=default;

	void OnStart(int argc, svctl::tchar_t** argv)
	{
		UNREFERENCED_PARAMETER(argc);
		UNREFERENCED_PARAMETER(argv);
		wchar_t temp[255];
		wsprintf(temp, L"MyService (0x%08X)::OnStart  argc=%d, argv[0] = %s\r\n", this, argc, argv[0]);
		OutputDebugString(temp);
	}

	void OnStop(void)
	{
	}

	void OnPause(void)
	{
		Sleep(15000);
	}

	// BEGIN_CONTROL_MAP()
	const svctl::control_handler_table& getHandlers(void) const
	{
		static std::unique_ptr<svctl::control_handler> handlers[] = { 
			std::make_unique<ServiceControlHandler<MyService>>(ServiceControl::Stop, &MyService::OnStop),
			std::make_unique<ServiceControlHandler<MyService>>(ServiceControl::Pause, &MyService::OnPause), 
			//std::make_unique<ServiceControlHandler<MyService>>(ServiceControl::Continue, &MyService::OnContinue) 
		};
		static svctl::control_handler_table v { std::make_move_iterator(std::begin(handlers)), std::make_move_iterator(std::end(handlers)) };
		return v;
	}

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

	// Manual dispatching with dynamic names
	ServiceTable services = { ServiceTableEntry<MyService>(L"MyService"), ServiceTableEntry<MyService>(L"MyService2") };
	services.Dispatch();
}

