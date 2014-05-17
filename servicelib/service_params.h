

#include "servicelib.h"

#ifndef __SERVICE_PARAMS_H_
#define __SERVICE_PARAMS_H_
#pragma once

#pragma warning(push, 4)

//////////////////////////////////////////////////////////////////////
/////////////// THIS WILL NOT WORK WITH DYNAMIC SERVICE NAMES 
/////////////////////////////////////////////////////////////////////

namespace svctl {

	// svctl::parameter
	//
	// Base implementation of a ::ServiceParameters<>::Parameter class
	class __declspec(novtable) parameter
	{
	public:

		virtual ~parameter()=default;

	protected:

		parameter()=default;

	private:

		parameter(const parameter&)=delete;
		parameter& operator=(const parameter&)=delete;
	};

	// svctl::parameter_map
	//
	// Collection of service-specific parameter objects
	typedef std::map<tstring, parameter&> parameter_map;

	// svctl::service_parameters
	//
	// Base implementation of the ::ServiceParameters<> class
	class __declspec(novtable) service_parameters : virtual private auxiliary_state_machine,
		private auxiliary_state
	{
	public:

		virtual ~service_parameters()=default;

	protected:

		service_parameters()
		{
			auxiliary_state_machine::RegisterAuxiliaryState(this);
		}

	private:

		// OnStart (auxiliary_state)
		//
		// TODO: words
		virtual void OnStart(const tchar_t* name, ServiceProcessType, int, tchar_t**);

		service_parameters(const service_parameters&)=delete;
		service_parameters& operator=(const service_parameters&)=delete;

		// m_keyhandle
		//
		// Parameters registry key handle
		HKEY m_keyhandle = nullptr;
	};

};	// namespace svctl

template <class _derived>
class ServiceParameters : private svctl::service_parameters
{
public:

	virtual ~ServiceParameters()=default;

protected:

	ServiceParameters()=default;

	template <typename _type, uint32_t _regtype = REG_NONE>
	class Parameter : private svctl::parameter
	{
		static_assert(std::is_pod<_type>::value, "Interesting error message goes here");

	public:

		Parameter(LPCTSTR name) { ServiceParameters<_derived>::BindParameter(name, *this); }
		Parameter(LPCTSTR name, _type default) : m_value(default) { 
			ServiceParameters<_derived>::BindParameter(name, *this); 
		}

		operator _type() { return m_value; }

		// && rvalue refs
		// & lvalue refs
		
		// probably need to always copy in/copy out? Inefficient for pointer types
		// but how to protect against refresh / SERVICE_CONTROL_PARAMCHANGE?

		_type& operator=(const _type& rhs)
		{
			m_value = rhs;
			return m_value;
		}



	private:

		Parameter(const Parameter&)=delete;
		Parameter& operator=(const Parameter&)=delete;
	
	public:

		_type m_value;
	};

//public:
//
//	template <typename _type>
//	static Parameter<_type> CreateParameter(LPCTSTR name)
//	{
//		return std::move(Parameter<_type>(name));
//	}

private:

	ServiceParameters(const ServiceParameters&)=delete;
	ServiceParameters& operator=(const ServiceParameters&)=delete;

	static void BindParameter(const svctl::tchar_t* name, svctl::parameter& param)
	{
		wchar_t temp[255];
		wsprintf(temp, L"BindParameter::param %s == 0x%08X\r\n", name, &param);
		OutputDebugString(temp);
		/*try { */s_params.insert(svctl::parameter_map::value_type(name, param)); /*}*/
		//catch(...) {
			/* DO SOMETHING */
		//}
	}

	// TODO: this can't be static, it has to be a member
	static svctl::parameter_map s_params;
};

template <class _derived>
svctl::parameter_map ServiceParameters<_derived>::s_params;

#pragma warning(pop)

#endif	// __SERVICE_PARAMS_H_

