// servicelib.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "servicelib.h"
#include "resource.h"
#include <thread>
#include <mutex>
#include <future>

struct __declspec(novtable) state
{
	virtual void AuxStart(int, svctl::tchar_t**)
	{
	}
};

class state_machine
{
public:

protected:

	void DoStuff(void)
	{
		OutputDebugString(L"DoStuff\r\n");
		for(auto i : m_states)
		{
			OutputDebugString(L"Doing stuff\r\n");
			i->AuxStart(0, nullptr);
		}
	}

	void RegisterState(state* instance)
	{
		OutputDebugString(L"Registering stuff\r\n");
		m_states.push_back(instance);
	}

private:

	std::vector<state*> m_states;
};

class auxone : private virtual state_machine, public state
{
public:

	auxone()
	{
		OutputDebugString(L"auxone::ctor\r\n");
		state_machine::RegisterState(static_cast<auxone*>(this));
	}

	void AuxStart(int, svctl::tchar_t**)
	{
		OutputDebugString(L"auxone::OnStart\r\n");
	}
};

class auxtwo : private virtual state_machine, public state
{
public:

	auxtwo()
	{
		OutputDebugString(L"auxtwo::ctor\r\n");
		state_machine::RegisterState(static_cast<auxtwo*>(this));
	}

	void AuxStart(int, svctl::tchar_t**)
	{
		OutputDebugString(L"auxtwo::OnStart\r\n");
	}
};

////////////////////////////////////////////////////


class MyService : public Service<MyService>, private virtual state_machine,
	public auxone, public auxtwo
{
public:

	MyService()=default;

	void OnStart(int argc, svctl::tchar_t** argv)
	{
		UNREFERENCED_PARAMETER(argc);
		UNREFERENCED_PARAMETER(argv);

		state_machine::DoStuff();

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

	// Manual dispatching with dynamic names
	ServiceTable services = { ServiceTableEntry<MyService>(L"MyService"), ServiceTableEntry<MyService>(L"MyService2") };
	services.Dispatch();
}

