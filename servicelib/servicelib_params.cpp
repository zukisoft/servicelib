
#include "stdafx.h"
#include "servicelib.h"

#pragma warning(push, 4)

namespace svctl {

	// bool
	template<> bool parameter::ReadValue<bool, ServiceParameterFormat::Auto>(void)
	{
		return ReadValue<bool, ServiceParameterFormat::DoubleWord>();
	}

	//template<> bool parameter::ReadValue<bool, ServiceParameterFormat::DoubleWord>(void)
	//{
		//return ReadValue<unsigned int, ServiceParameterFormat::DoubleWord>() != 0;
		//return false;
	//}

	template<> int parameter::ReadValue<int, ServiceParameterFormat::Auto>(void)
	{
		return -1;
	}
		
	template<> unsigned long parameter::ReadValue<unsigned long, ServiceParameterFormat::Auto>(void)
	{
		return 1;
	}
		
	template<> unsigned int parameter::ReadValue<unsigned int, ServiceParameterFormat::Auto>(void)
	{
		return 1;
	}
};

#pragma warning(pop)
