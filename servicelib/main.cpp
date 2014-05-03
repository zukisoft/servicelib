// servicelib.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "servicelib.h"
#include "resource.h"

////////////////////////////////////////////////////

class MyService : public Service<MyService>
{
public:

	virtual uint32_t Run(uint32_t argc, svctl::tchar_t** argv)
	{
		return 0;
	}

};

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{
	ServiceModule<MyService, MyService, MyService> module;
	return module.WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}


