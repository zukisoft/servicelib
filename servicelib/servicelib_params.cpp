
#include "stdafx.h"
#include "servicelib.h"

#pragma warning(push, 4)

namespace svctl {

	// bool
	template<> bool parameter::ReadValue<bool, ServiceParameterFormat::Auto>(void)
	{
		size_t length;
		ServiceParameterFormat type;
		void* data = ReadValue(&length, &type);
		if(!data) return false;

		switch(type) {
			case ServiceParameterFormat::Binary:
			case ServiceParameterFormat::DoubleWord:
			case ServiceParameterFormat::ExpandString:
			case ServiceParameterFormat::QuadWord:
			case ServiceParameterFormat::String:
			case ServiceParameterFormat::StringArray:
			default:
				return false;
		}
	}

	//template<> bool parameter::ReadValue<bool, ServiceParameterFormat::DoubleWord>(void)
	//{
	//	return ReadValue<bool, ServiceParameterFormat::Auto>();
	//	//return ReadValue<unsigned int, ServiceParameterFormat::DoubleWord>() != 0;
	//	//return false;
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
