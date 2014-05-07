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

	uint32_t Initialize(uint32_t argc, svctl::tchar_t** argv)
	{
		return NO_ERROR;
	}

	uint32_t Run()
	{
		return 0;
	}

	void Terminate(void) {}
};

////////////////////////////////////////////////////////////////
typedef svctl::tchar_t tchar_t;

//namespace svctl {

	struct service_state
	{
		virtual void OnInitialize(void) = 0;
		//virtual void OnTerminate(void) = 0;
	};


	class service_state_machine
	{
	public:

		void Initialize(void);

		//void Terminate(void);

	protected:

		void RegisterInstance(service_state* instance);

	private:

		// m_instances
		//
		// Instances of service_state interface registered with the state machine
		std::vector<service_state*>		m_instances;
	};

	// cpp

	void service_state_machine::Initialize(void)
	{
		for(const auto& iterator : m_instances) iterator->OnInitialize();
	}

	//void service_state_machine::Terminate(void)
	//{
	//	for(const auto& iterator : m_instances) iterator->OnTerminate();
	//}

	void service_state_machine::RegisterInstance(service_state* instance)
	{
		m_instances.push_back(instance);		// Add to collection
	}


	class service_events : public service_state, virtual private service_state_machine
	{
	public:
		service_events()
		{
			// Register this class with the state machine
			service_state_machine::RegisterInstance(this);
		}

		virtual void OnInitialize(void) { OutputDebugString(L"service_events::OnInitialize\r\n"); }
	};

	class service_parameters : public service_state, virtual private service_state_machine
	{
	public:
		service_parameters()
		{
			// Register this class with the state machine
			service_state_machine::RegisterInstance(this);
		}

		virtual void OnInitialize(void) { OutputDebugString(L"service_parameters::OnInitialize\r\n"); }
	};

	class service : virtual public service_state_machine
	{
	};


	template <class _derived>
	class ServiceEvents : public service_events
	{
	};

	template <class _derived>
	class Test : public service
	{
	};

	class MyTest : public Test<MyTest>, public ServiceEvents<MyTest>	
	{
	};

// }


/////////////////////////////////////////////////////////////////

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{

#ifdef _DEBUG

	int nDbgFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);	// Get current flags
	nDbgFlags |= _CRTDBG_LEAK_CHECK_DF;						// Enable leak-check
	_CrtSetDbgFlag(nDbgFlags);								// Set the new flags

#endif	// _DEBUG

	//int val = 123;
	//TestFunc myfunc = [=]() -> int { return val; };

	//int x = myfunc();
	//int y = myfunc();

	//MyTest myservice;
	//myservice.Initialize();

	ServiceModule<MyService> module;
	return module.WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}


