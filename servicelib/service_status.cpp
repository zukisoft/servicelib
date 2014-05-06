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

service_status_manager::service_status_manager(SERVICE_STATUS_HANDLE handle, report_status_t func) : 
	m_reportfunc(func), m_handle(handle)
{
	//m_event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
}

service_status_manager::~service_status_manager()
{
	//CloseHandle(m_event);
}

//-----------------------------------------------------------------------------
// service_status_manager::CancelPendingStatus (private)
//
// Cancels a pending status worker thread operation, if running
//
// Arguments:
//
//	NONE
//
// >> NOTE: should be called with m_cs already entered <<
bool service_status_manager::CancelPendingStatus(void)
{
	if(!m_worker.joinable()) return false;

	// Signal the event object to stop the pending status thread and wait
	if(SignalObjectAndWait(m_event, m_worker.native_handle(), INFINITE, FALSE) == WAIT_FAILED)
		throw winexception();

	m_worker.join();
	//CloseHandle(m_worker);				// Close the worker thread object
	//m_worker = nullptr;					// Reset worker thread pointer to NULL
	ResetEvent(m_event);				// Reset event object signal state
	
	return true;
}

//-----------------------------------------------------------------------------
// service_status_manager::DeriveAcceptedControlsMask (private, static)
//
// Converts a SERVICE_CONTROL_XXX mask into a SERVICE_ACCEPT_XXX mask
//
// Arguments:
//
//	controls	- Mask of SERVICE_CONTROL_XXX flags to be converted

uint32_t service_status_manager::DeriveAcceptedControlsMask(uint32_t controls)
{
	uint32_t mask = 0;

	if(controls & SERVICE_CONTROL_STOP)						mask |= SERVICE_ACCEPT_STOP;
	if(controls & SERVICE_CONTROL_PAUSE)					mask |= SERVICE_ACCEPT_PAUSE_CONTINUE;
	if(controls & SERVICE_CONTROL_CONTINUE)					mask |= SERVICE_ACCEPT_PAUSE_CONTINUE;
	if(controls & SERVICE_CONTROL_SHUTDOWN)					mask |= SERVICE_ACCEPT_SHUTDOWN;
	if(controls & SERVICE_CONTROL_PARAMCHANGE)				mask |= SERVICE_ACCEPT_PARAMCHANGE;
	if(controls & SERVICE_CONTROL_NETBINDADD)				mask |= SERVICE_ACCEPT_NETBINDCHANGE;
	if(controls & SERVICE_CONTROL_NETBINDREMOVE)			mask |= SERVICE_ACCEPT_NETBINDCHANGE;
	if(controls & SERVICE_CONTROL_NETBINDENABLE)			mask |= SERVICE_ACCEPT_NETBINDCHANGE;
	if(controls & SERVICE_CONTROL_NETBINDDISABLE)			mask |= SERVICE_ACCEPT_NETBINDCHANGE;
	if(controls & SERVICE_CONTROL_HARDWAREPROFILECHANGE)	mask |= SERVICE_ACCEPT_HARDWAREPROFILECHANGE;
	if(controls & SERVICE_CONTROL_POWEREVENT)				mask |= SERVICE_ACCEPT_POWEREVENT;
	if(controls & SERVICE_CONTROL_SESSIONCHANGE)			mask |= SERVICE_ACCEPT_SESSIONCHANGE;
	if(controls & SERVICE_CONTROL_PRESHUTDOWN)				mask |= SERVICE_ACCEPT_PRESHUTDOWN;
	if(controls & SERVICE_CONTROL_TIMECHANGE)				mask |= SERVICE_ACCEPT_TIMECHANGE;
	if(controls & SERVICE_CONTROL_TRIGGEREVENT)				mask |= SERVICE_ACCEPT_TRIGGEREVENT;
	if(controls & SERVICE_CONTROL_USERMODEREBOOT)			mask |= SERVICE_ACCEPT_USERMODEREBOOT;

	return mask;
}

void service_status_manager::Set(ServiceStatus status, uint32_t win32exitcode, uint32_t serviceexitcode)
{
	switch(status) {

		// Pending status codes
		//
		case ServiceStatus::StartPending:
		case ServiceStatus::StopPending:
		case ServiceStatus::ContinuePending:
		case ServiceStatus::PausePending:
			SetPendingStatus(status);
			return;

		// Non-pending status codes without exit codes
		//
		case ServiceStatus::Running:
		case ServiceStatus::Paused:
			SetNonPendingStatus(status);
			return;

		// Stopped - expose the exit codes
		case ServiceStatus::Stopped:
			SetNonPendingStatus(status, win32exitcode, serviceexitcode);
			return;

		default: throw winexception(E_INVALIDARG);
	}
}

void service_status_manager::SetNonPendingStatus(ServiceStatus status, uint32_t win32exitcode, uint32_t serviceexitcode)
{
	auto_cs lock(m_cs);
	CancelPendingStatus();
	//if(m_worker) { 

	//	SignalObjectAndWait(m_event, m_worker, INFINITE, FALSE);
	//	CloseHandle(m_worker);
	//	m_worker = nullptr;
	//}
}

void service_status_manager::SetPendingStatus(ServiceStatus status)
{
	auto_cs lock(m_cs);
	CancelPendingStatus();
	//if(m_worker) {

	//	SignalObjectAndWait(m_event, m_worker, INFINITE, FALSE);
	//	CloseHandle(m_worker);
	//	m_worker = nullptr;
	//}

	//ResetEvent(m_event);
	//_beginthreadex(nullptr, blah);
}

//-----------------------------------------------------------------------------

END_NAMESPACE(svctl)

//-----------------------------------------------------------------------------

#pragma pop_macro("END_NAMESPACE")
#pragma pop_macro("BEGIN_NAMESPACE")
#pragma warning(pop)
