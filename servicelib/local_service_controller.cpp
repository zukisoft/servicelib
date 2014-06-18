#include "stdafx.h"
#include "servicelib.h"
#pragma warning(push, 4)

namespace svctl {

//-----------------------------------------------------------------------------
// svctl::local_service_controller
//-----------------------------------------------------------------------------

std::map<tstring, std::unique_ptr<local_service>, local_service_controller::mycomp> 
local_service_controller::s_services;

///
//  todo: words
local_service::operator SERVICE_STATUS_HANDLE() const
{
	// The SERVICE_STATUS_HANDLE is merely the address of this class instance
	return reinterpret_cast<SERVICE_STATUS_HANDLE>(const_cast<local_service*>(this)); 
}

//-----------------------------------------------------------------------------
// local_service_controller::RegisterControlHandler (static)
//
// Mimicks the global ::RegisterServiceCtrlHandlerEx function in order to register
// a service as a local application using the same basic service template code
//
// Arguments:
//
//	servicename		- Service name
//	handler			- Service control handler callback function
//	context			- Context to be provided to the handler callback function

SERVICE_STATUS_HANDLE local_service_controller::RegisterControlHandler(LPCTSTR servicename, LPHANDLER_FUNCTION_EX handler, LPVOID context)
{
	// TODO: error codes that can be set with SetLastError(), return a nullptr
	//
	// ERROR_SERVICE_NOT_IN_EXE --> bad service name
	// ERROR_NOT_ENOUGH_MEMORY --> can't allocate service entry
	// ERROR_INVALID_DATA --> bad handler

	std::unique_ptr<local_service> service = std::make_unique<local_service>(handler, context);
	SERVICE_STATUS_HANDLE result = service->operator SERVICE_STATUS_HANDLE();

	s_services.insert(std::make_pair(servicename, std::move(service)));

	return result;
}

//-----------------------------------------------------------------------------
// local_service_controller::SetStatus (static)
//
// Mimicks the global ::SetServiceStatus function in order to allow a service
// running as a local application to report status
//
// Arguments:
//
//	handle			- Handle returned from RegisterControlHandler
//	status			- Service status to be reported

BOOL local_service_controller::SetStatus(SERVICE_STATUS_HANDLE handle, LPSERVICE_STATUS status)
{
	_ASSERTE(handle != nullptr);
	_ASSERTE(status != nullptr);

	// todo: also check handle is valid against collection --> ERROR_INVALID_HANDLE
	if(!handle) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }

	// todo: also check status data is valid --> ERROR_INVALID_DATA
	if(!status) { SetLastError(ERROR_INVALID_DATA); return FALSE; }

	// todo: won't really do it like this, just testing .. need to actually search
	// the collection for the service ... should I maybe have used a set<> instead?
	local_service* service = reinterpret_cast<local_service*>(handle);
	service->Status = *status;

	return TRUE;
}

};

#pragma warning(pop)
