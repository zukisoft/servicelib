
#include "stdafx.h"
#include "servicelib.h"

#pragma warning(push, 4)

namespace svctl {

	// bool

	// char

	// wchar_t

	// unsigned long --> 4

	// unsigned long long --> 8

	// float --> reinterpret unsigned long

	// double --> reinterpret unsigned long long

	// bool --> unsigned long
	template<> bool parameter::ReadValue<bool, ServiceParameterFormat::Auto>(void)
	{
		unsigned long value = ReadValue<unsigned long, ServiceParameterFormat::Auto>();
		return (value != 0);
	}

	// short --> unsigned long
	template<> short parameter::ReadValue<short, ServiceParameterFormat::Auto>(void) 
		{ return static_cast<short>(ReadValue<unsigned long, ServiceParameterFormat::Auto>()); }

	// unsigned short --> unsigned long
	template<> unsigned short parameter::ReadValue<unsigned short, ServiceParameterFormat::Auto>(void) 
		{ return static_cast<unsigned short>(ReadValue<unsigned long, ServiceParameterFormat::Auto>()); }

	// int --> unsigned long
	template<> int parameter::ReadValue<int, ServiceParameterFormat::Auto>(void) 
		{ return static_cast<int>(ReadValue<unsigned long, ServiceParameterFormat::Auto>()); }

	// unsigned int --> unsigned long
	template<> unsigned int parameter::ReadValue<unsigned int, ServiceParameterFormat::Auto>(void) 
		{ return static_cast<unsigned int>(ReadValue<unsigned long, ServiceParameterFormat::Auto>()); }
		
	// long --> unsigned long
	template<> long parameter::ReadValue<long, ServiceParameterFormat::Auto>(void) 
		{ return static_cast<long>(ReadValue<unsigned long, ServiceParameterFormat::Auto>()); }
		
	template<> unsigned long parameter::ReadValue<unsigned long, ServiceParameterFormat::Auto>(void)
	{
		size_t					rawlength;			// Length of the raw registry value
		ServiceParameterFormat	rawtype;			// Type/format of the raw registry value
		unsigned long			value = 0;			// Resultant parameter value

		// Read the raw registry value, length and type information from the registry
		void* raw = ReadValue(&rawlength, &rawtype);
		if(!raw) return value;

		switch(rawtype) {

			// Binary / DoubleWord / QuadWord --> Cast
			case ServiceParameterFormat::Binary:
			case ServiceParameterFormat::DoubleWord:
			case ServiceParameterFormat::QuadWord:
				if(rawlength >= sizeof(unsigned int)) value = *reinterpret_cast<unsigned long*>(raw);
				break;

			// String / ExpandString --> Convert
			case ServiceParameterFormat::String:
			case ServiceParameterFormat::ExpandString:
				// TODO
				break;

			// StringArray --> Not Supported
			case ServiceParameterFormat::StringArray:

				// TODO
				break;
		}

		FreeValue(raw);						// Release heap allocated buffer
		return value;						// Return converted value
	}
};

#pragma warning(pop)
