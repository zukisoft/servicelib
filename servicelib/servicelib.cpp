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

//-----------------------------------------------------------------------------

BEGIN_NAMESPACE(svctl)

//-----------------------------------------------------------------------------
// svctl::auxiliary_state_machine
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// auxiliary_state_machine::Initialize
//
// Invoked to initialize all registered virtual auxiliary base classes
//
// Arguments:
//
//	servicename		- Name associated with the derived service

void auxiliary_state_machine::Initialize(const tchar_t* servicename)
{
	_ASSERTE(servicename != nullptr);
	if(!servicename) throw winexception(E_INVALIDARG);

	// Invoke auxiliary_state::OnInitialize for all registered instances
	for(const auto& iterator : m_instances) iterator->OnInitialize(servicename);
}

//-----------------------------------------------------------------------------
// auxiliary_state_machine::RegisterAuxiliaryState (protected)
//
// Registers an auxiliary class with the state machine.  This should be called
// from the constructor of an auxiliary class that derives from auxiliary_state_machine
// as a virtual private base class
//
// Arguments:
//
//	instance		- auxiliary_state interface instance pointer

void auxiliary_state_machine::RegisterAuxiliaryState(struct auxiliary_state* instance)
{
	_ASSERTE(instance != nullptr);
	if(!instance) throw winexception(E_INVALIDARG);

	m_instances.push_back(instance);
}

//-----------------------------------------------------------------------------
// svctl::resstring
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// resstring::GetResourceString (private, static)
//
// Gets a read-only pointer to a module string resource
//
// Arguments:
//
//	id			- Resource identifier code
//	instance	- Module instance handle to acquire the resource from

const tchar_t* resstring::GetResourceString(unsigned int id, HINSTANCE instance)
{
	static const tchar_t EMPTY[] = _T("");		// Used for missing resources

	// LoadString() has a neat trick to return a read-only string pointer
	tchar_t* string = nullptr;
	int result = LoadString(instance, id, reinterpret_cast<tchar_t*>(&string), 0);

	// Return an empty string rather than a null pointer if the resource was missing
	return (result == 0) ? EMPTY : string;
}

//-----------------------------------------------------------------------------
// svctl::service
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// svctl::winexception
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// winexception Constructor
//
// Arguments:
//
//	result		- Win32 error code

winexception::winexception(DWORD result) : m_code(result)
{
	char_t*				formatted;				// Formatted message

	// Invoke FormatMessageA to convert the system error code into an ANSI string; use a lame
	// generic 'unknown' string for any codes that cannot be looked up successfully
	if(FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, result,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<char_t*>(&formatted), 0, nullptr)) {
		
		m_what = formatted;						// Store the formatted message string
		LocalFree(formatted);					// Release FormatMessage() allocated buffer
	}

	else m_what = "Unknown Windows status code " + std::to_string(result);
}

//-----------------------------------------------------------------------------

END_NAMESPACE(svctl)

//-----------------------------------------------------------------------------

#pragma pop_macro("END_NAMESPACE")
#pragma pop_macro("BEGIN_NAMESPACE")
#pragma warning(pop)
