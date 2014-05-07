//-----------------------------------------------------------------------------
// Copyright (c) 2001-2014 Michael G. Brehm
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "servicelib.h"

#pragma warning(push, 4)

// BEGIN_NAMESPACE / END_NAMESPACE
//
#pragma push_macro("BEGIN_NAMESPACE")
#pragma push_macro("END_NAMESPACE")
#undef BEGIN_NAMESPACE
#undef END_NAMESPACE
#define BEGIN_NAMESPACE(ns)		namespace ns {
#define END_NAMESPACE(ns)		}

//// 

// THIS FILE GETS MERGED WITH THE MAIN CPP FILE

//-----------------------------------------------------------------------------

BEGIN_NAMESPACE(svctl)

//-----------------------------------------------------------------------------
// svctl::service_status_manager
//-----------------------------------------------------------------------------

////-----------------------------------------------------------------------------
//// service_status_manager::DeriveAcceptedControlsMask (private, static)
////
//// Converts a SERVICE_CONTROL_XXX mask into a SERVICE_ACCEPT_XXX mask
////
//// Arguments:
////
////	controls	- Mask of SERVICE_CONTROL_XXX flags to be converted
//
//uint32_t service_status_manager::DeriveAcceptedControlsMask(uint32_t controls)
//{
//	uint32_t mask = 0;
//
//	if(controls & SERVICE_CONTROL_STOP)						mask |= SERVICE_ACCEPT_STOP;
//	if(controls & SERVICE_CONTROL_PAUSE)					mask |= SERVICE_ACCEPT_PAUSE_CONTINUE;
//	if(controls & SERVICE_CONTROL_CONTINUE)					mask |= SERVICE_ACCEPT_PAUSE_CONTINUE;
//	if(controls & SERVICE_CONTROL_SHUTDOWN)					mask |= SERVICE_ACCEPT_SHUTDOWN;
//	if(controls & SERVICE_CONTROL_PARAMCHANGE)				mask |= SERVICE_ACCEPT_PARAMCHANGE;
//	if(controls & SERVICE_CONTROL_NETBINDADD)				mask |= SERVICE_ACCEPT_NETBINDCHANGE;
//	if(controls & SERVICE_CONTROL_NETBINDREMOVE)			mask |= SERVICE_ACCEPT_NETBINDCHANGE;
//	if(controls & SERVICE_CONTROL_NETBINDENABLE)			mask |= SERVICE_ACCEPT_NETBINDCHANGE;
//	if(controls & SERVICE_CONTROL_NETBINDDISABLE)			mask |= SERVICE_ACCEPT_NETBINDCHANGE;
//	if(controls & SERVICE_CONTROL_HARDWAREPROFILECHANGE)	mask |= SERVICE_ACCEPT_HARDWAREPROFILECHANGE;
//	if(controls & SERVICE_CONTROL_POWEREVENT)				mask |= SERVICE_ACCEPT_POWEREVENT;
//	if(controls & SERVICE_CONTROL_SESSIONCHANGE)			mask |= SERVICE_ACCEPT_SESSIONCHANGE;
//	if(controls & SERVICE_CONTROL_PRESHUTDOWN)				mask |= SERVICE_ACCEPT_PRESHUTDOWN;
//	if(controls & SERVICE_CONTROL_TIMECHANGE)				mask |= SERVICE_ACCEPT_TIMECHANGE;
//	if(controls & SERVICE_CONTROL_TRIGGEREVENT)				mask |= SERVICE_ACCEPT_TRIGGEREVENT;
//	if(controls & SERVICE_CONTROL_USERMODEREBOOT)			mask |= SERVICE_ACCEPT_USERMODEREBOOT;
//
//	return mask;
//}
//
//-----------------------------------------------------------------------------
// service_status_manager::Set
//
// Sets a new service status; pending vs. non-pending is handled automatically
//
// Arguments:
//
//	status			- New service status to set
//	win32exitcode	- Win32 specific exit code for ServiceStatus::Stopped (see documentation)
//	serviceexitcode	- Service-specific exit code for ServiceStatus::Stopped (see documentation)

void service_status_manager::Set(ServiceStatus status, uint32_t win32exitcode, uint32_t serviceexitcode)
{
	lock critsec(m_cs);						// Automatic CRITICAL_SECTION wrapper

	// Check for a duplicate status; pending states are managed automatically
	if(status == m_status) return;

	// Check for a pending service state operation
	if(m_worker.joinable()) {

		// Cancel the pending state checkpoint thread by signaling the event object
		if(SignalObjectAndWait(m_signal, m_worker.native_handle(), INFINITE, FALSE) == WAIT_FAILED) throw winexception();

		m_worker.join();
		m_signal.Reset();
	}

	// Invoke the proper status helper based on the type of status being set
	switch(status) {

		// Pending status codes
		case ServiceStatus::StartPending:
		case ServiceStatus::StopPending:
		case ServiceStatus::ContinuePending:
		case ServiceStatus::PausePending:
			SetPendingStatus_l(status);
			break;

		// Non-pending status codes without an exit status
		case ServiceStatus::Running:
		case ServiceStatus::Paused:
			SetNonPendingStatus_l(status);
			break;

		// Non-pending status codes that report exit status
		case ServiceStatus::Stopped:
			SetNonPendingStatus_l(status, win32exitcode, serviceexitcode);
			break;

		// Invalid status code
		default: throw winexception(E_INVALIDARG);
	}
	
	m_status = status;					// Service status has been changed
}

//-----------------------------------------------------------------------------
// service_status_manager::SetNonPendingStatus_l (private)
//
// Sets a non-pending service status
//
// Arguments:
//
//	status				- New service status to set
//	win32exitcode		- Win32 service exit code for ServiceStatus::Stopped (see documentation)
//	serviceexitcode		- Service specific exit code for ServiceStatus::Stopped (see documentation)
//
// CAUTION: m_cs should be locked before calling this function
void service_status_manager::SetNonPendingStatus_l(ServiceStatus status, uint32_t win32exitcode, uint32_t serviceexitcode)
{
	_ASSERTE(!m_worker.joinable());					// Should not be running

	SERVICE_STATUS newstatus;
	newstatus.dwServiceType = static_cast<DWORD>(m_processtype);
	newstatus.dwCurrentState = static_cast<DWORD>(status);
	newstatus.dwControlsAccepted = (status == ServiceStatus::Stopped) ? 0 : m_acceptedcontrols;
	newstatus.dwWin32ExitCode = (status == ServiceStatus::Stopped) ? win32exitcode : ERROR_SUCCESS;
	newstatus.dwServiceSpecificExitCode = (status == ServiceStatus::Stopped) ? serviceexitcode : ERROR_SUCCESS;
	newstatus.dwCheckPoint = 0;
	newstatus.dwWaitHint = 0;
	m_reportfunc(m_statushandle, newstatus);
}

//-----------------------------------------------------------------------------
// service_status_manager::SetPendingStatus_l (private)
//
// Sets an automatically checkpointed pending service status
//
// Arguments:
//
//	status				- Pending service status to set
//
// CAUTION: m_cs should be locked before calling this function
void service_status_manager::SetPendingStatus_l(ServiceStatus status)
{
	_ASSERTE(!m_worker.joinable());					// Should not be running

	// Disallow all service controls during START_PENDING and STOP_PENDING status transitions, otherwise
	// only filter out additional pending controls.  Dealing with controls received during PAUSE and CONTINUE
	// operations may or may not be valid based on the service, so that's left up to the implementation
	uint32_t accept = ((status == ServiceStatus::StartPending) || (status == ServiceStatus::StopPending)) ? 0 :
		(m_acceptedcontrols & ~(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE));

	// Kick off a new worker thread to manage the automatic checkpoint operation
	m_worker = std::thread([&]() {

		SERVICE_STATUS pendingstatus;
		pendingstatus.dwServiceType = static_cast<DWORD>(m_processtype);
		pendingstatus.dwCurrentState = static_cast<DWORD>(status);
		pendingstatus.dwControlsAccepted = accept;
		pendingstatus.dwWin32ExitCode = ERROR_SUCCESS;
		pendingstatus.dwServiceSpecificExitCode = ERROR_SUCCESS;
		pendingstatus.dwCheckPoint = 1;
		pendingstatus.dwWaitHint = (status == ServiceStatus::StartPending) ? STARTUP_WAIT_HINT : PENDING_WAIT_HINT;
		m_reportfunc(m_statushandle, pendingstatus);

		// Continually report the same pending status with an incremented checkpoint until signaled
		while(WaitForSingleObject(m_signal, PENDING_CHECKPOINT_INTERVAL) == WAIT_TIMEOUT) {

			++pendingstatus.dwCheckPoint;
			m_reportfunc(m_statushandle, pendingstatus);
		}
	});
}

//-----------------------------------------------------------------------------

END_NAMESPACE(svctl)

#pragma pop_macro("END_NAMESPACE")
#pragma pop_macro("BEGIN_NAMESPACE")
#pragma warning(pop)
