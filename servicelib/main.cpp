// servicelib.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "servicelib.h"
#include "resource.h"

#include <array>
#include <vector>

////////////////////////////////////////////////////

class MyService : public Service<MyService>
{
public:

	virtual uint32_t Run(uint32_t argc, svctl::tchar_t** argv)
	{
		return 0;
	}

};

//template<typename... _services>
//class Test
//{
//public:
//
//	Test() 
//	{
//		m_services = std::initializer_list<svctl::service_config> { (_services::getConfiguration())... };
//	}
//
//private:
//
//	std::vector<svctl::service_config>	m_services;
//};
//
//Test<MyService, MyService> mytest;

//svctl::service_config& config = MyService::getConfiguration();

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	ServiceModule<MyService> module;
	int x =  module.WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);

	return x;


	//UNREFERENCED_PARAMETER(hPrevInstance);
	//UNREFERENCED_PARAMETER(lpCmdLine);

	//MyService::StaticMain(12, nullptr);

	////int x = 3;
	////std::vector<svctl::service_table_entry_t> table { svctl::service_table_entry_t(nullptr, nullptr) };
	////std::vector<MYSTRUCT> structs { { 1, 2 }, { 3, 4 } };
	////MYSTRUCT* p = structs.data();


	////MyService::s_instance = std::make_unique<Service<MyService>>();
	////MyService::ServiceMain(0, nullptr);

 //	// TODO: Place code here.
	//MSG msg;
	//HACCEL hAccelTable;

	//// Initialize global strings
	//LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	//LoadString(hInstance, IDC_SERVICELIB, szWindowClass, MAX_LOADSTRING);
	//MyRegisterClass(hInstance);

	////svctl::service_map map = {

	////	svctl::service_map_entry_t<MyService>(),
	////	//svctl::service_table_entry_t<MyService>()
	////};

	////std::initializer_list<svctl::service_map_entry> il1 = {};
	////std::initializer_list<svctl::service_map_entry> il2 = { svctl::service_map_entry_t<MyService>() };

	////LPSERVICE_TABLE_ENTRY ptable = table.data();

	//// Perform application initialization:
	//if (!InitInstance (hInstance, nCmdShow))
	//{
	//	return FALSE;
	//}

	//hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SERVICELIB));

	//// Main message loop:
	//while (GetMessage(&msg, NULL, 0, 0))
	//{
	//	if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
	//	{
	//		TranslateMessage(&msg);
	//		DispatchMessage(&msg);
	//	}
	//}

	//return (int) msg.wParam;
}


