// servicelib.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "servicelib.h"
#include "resource.h"
#include <thread>
#include <mutex>

////////////////////////////////////////////////////

class MyService : public Service<MyService>
{
public:

	MyService()=default;

	virtual void OnStart(uint32_t argc, svctl::tchar_t** argv)
	{
		UNREFERENCED_PARAMETER(argc);
		UNREFERENCED_PARAMETER(argv);
	}

	void OnStop(void)
	{
		Sleep(10000);
	}

	virtual const std::vector<std::unique_ptr<svctl::control_handler>>& getHandlers(void)
	{
		static std::unique_ptr<svctl::control_handler> handlers[] = { std::make_unique<InlineControlHandler<MyService>>(ServiceControl::Stop, &MyService::OnStop) };
		static std::vector<std::unique_ptr<svctl::control_handler>> v { std::make_move_iterator(std::begin(handlers)), std::make_move_iterator(std::end(handlers)) };
		return v;
	}

private:

	MyService(const MyService&)=delete;
	MyService& operator=(const MyService&)=delete;

};

///////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{

#ifdef _DEBUG

	int nDbgFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);	// Get current flags
	nDbgFlags |= _CRTDBG_LEAK_CHECK_DF;						// Enable leak-check
	_CrtSetDbgFlag(nDbgFlags);								// Set the new flags

#endif	// _DEBUG

	ServiceModule<MyService> module;
	return module.WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}


////////////////////////////////////////////////////////////////
//typedef svctl::tchar_t tchar_t;
//
////namespace svctl {
//
//	struct service_state
//	{
//		virtual void OnInitialize(void) = 0;
//		//virtual void OnTerminate(void) = 0;
//	};
//
//
//	class service_state_machine
//	{
//	public:
//
//		void Initialize(void);
//
//		//void Terminate(void);
//
//	protected:
//
//		void RegisterInstance(service_state* instance);
//
//	private:
//
//		// m_instances
//		//
//		// Instances of service_state interface registered with the state machine
//		std::vector<service_state*>		m_instances;
//	};
//
//	// cpp
//
//	void service_state_machine::Initialize(void)
//	{
//		for(const auto& iterator : m_instances) iterator->OnInitialize();
//	}
//
//	//void service_state_machine::Terminate(void)
//	//{
//	//	for(const auto& iterator : m_instances) iterator->OnTerminate();
//	//}
//
//	void service_state_machine::RegisterInstance(service_state* instance)
//	{
//		m_instances.push_back(instance);		// Add to collection
//	}
//
//
//	class service_events : public service_state, virtual private service_state_machine
//	{
//	public:
//		service_events()
//		{
//			// Register this class with the state machine
//			service_state_machine::RegisterInstance(this);
//		}
//
//		virtual void OnInitialize(void) { OutputDebugString(L"service_events::OnInitialize\r\n"); }
//	};
//
//	class service_parameters : public service_state, virtual private service_state_machine
//	{
//	public:
//		service_parameters()
//		{
//			// Register this class with the state machine
//			service_state_machine::RegisterInstance(this);
//		}
//
//		virtual void OnInitialize(void) { OutputDebugString(L"service_parameters::OnInitialize\r\n"); }
//	};
//
//	class service : virtual public service_state_machine
//	{
//	};
//
//
//	template <class _derived>
//	class ServiceEvents : public service_events
//	{
//	};
//
//	template <class _derived>
//	class Test : public service
//	{
//	};
//
//	class MyTest : public Test<MyTest>, public ServiceEvents<MyTest>	
//	{
//	};
//
//// }
//
//
