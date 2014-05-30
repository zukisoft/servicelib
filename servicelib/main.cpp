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

	DWORD OnMyCommand(void)
	{
		OutputDebugString(L"MyService::OnMyCommand!\r\n");
		return ERROR_SUCCESS;
	}

	BEGIN_CONTROL_HANDLER_MAP(MyService)
		CONTROL_HANDLER(ServiceControl::Stop, OnStop)
		CONTROL_HANDLER(ServiceControl::Pause, OnPause)
		CONTROL_HANDLER(200, OnMyCommand)
	END_CONTROL_HANDLER_MAP()

private:

	MyService(const MyService&)=delete;
	MyService& operator=(const MyService&)=delete;

	HANDLE mywait;

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

