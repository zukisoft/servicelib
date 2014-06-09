
#include "stdafx.h"
#include "servicelib.h"

#pragma warning(push, 4)

namespace svctl {

	// svctl::parameter_format_string
	//
	template <typename _test, typename _type>
	using parameter_format_string = typename std::enable_if<std::is_same<_type, _test>::value, const tchar_t*>::type;

	// svctl::ParameterFormatString
	//
	// Defines the format string that is passed to sscanf_s() to convert a registry string into a fundamental type.
	// Uses compile-time enable_if<> to ensure that any missing formats will generate a compile-time error

#ifdef _UNICODE
	template <typename _type> parameter_format_string<_type, char>				ParameterFormatString(void) { return _T("%C"); }
	template <typename _type> parameter_format_string<_type, __wchar_t>			ParameterFormatString(void) { return _T("%c"); }
#else
	template <typename _type> parameter_format_string<_type, char>				ParameterFormatString(void) { return _T("%c"); }
	template <typename _type> parameter_format_string<_type, __wchar_t>			ParameterFormatString(void) { return _T("%C"); }
#endif

	template <typename _type> parameter_format_string<_type, signed char>		ParameterFormatString(void) { return _T("%hi"); }	// TODO: could overflow
	template <typename _type> parameter_format_string<_type, unsigned char>		ParameterFormatString(void) { return _T("%hu"); }	// TODO: could overflow
	template <typename _type> parameter_format_string<_type, short>				ParameterFormatString(void) { return _T("%hi"); }
	template <typename _type> parameter_format_string<_type, unsigned short>	ParameterFormatString(void) { return _T("%hu"); }
	template <typename _type> parameter_format_string<_type, int>				ParameterFormatString(void) { return _T("%i"); }
	template <typename _type> parameter_format_string<_type, unsigned int>		ParameterFormatString(void) { return _T("%u"); }
	template <typename _type> parameter_format_string<_type, long>				ParameterFormatString(void) { return _T("%li"); }
	template <typename _type> parameter_format_string<_type, unsigned long>		ParameterFormatString(void) { return _T("%lu"); }
	template <typename _type> parameter_format_string<_type, long long>			ParameterFormatString(void) { return _T("%lli"); }
	template <typename _type> parameter_format_string<_type, float>				ParameterFormatString(void) { return _T("%f"); }
	template <typename _type> parameter_format_string<_type, double>			ParameterFormatString(void) { return _T("%d"); }
	template <typename _type> parameter_format_string<_type, long double>		ParameterFormatString(void) { return _T("%ld"); }

	template <typename _type>
	typename std::enable_if<std::is_integral<_type>::value, _type>::type
	ConvertParameter(void *raw, size_t length, DWORD type)
	{
		const tchar_t* format = ParameterFormatString<_type>();
	}

	//template <typename _type>
	//typename std::enable_if<std::is_fundamental<_type>::value, _type>::type
	//RegStringToFundamental(void* raw, size_t length, DWORD type, const tchar_t* scanformat)
	//{
	//	// converts a string into a fundamental type
	//}

	template<typename _type>
	typename std::enable_if<std::is_integral<_type>::value, _type>::type
	ConvertRegistryValue(void* raw, size_t length, DWORD type, const tchar_t* scanformat)
	{
		if(type == REG_BINARY || type == REG_DWORD || type == REG_QWORD) {

			if(length >= sizeof(double)) return static_cast<_type>(*reinterpret_cast<double*>(raw));
			else if(length >= sizeof(float)) return static_cast<_type>(*reinterpret_cast<float*>(raw));
			else return static_cast<_type>(0);
		}

		else if(type == REG_SZ || type == REG_EXPAND_SZ) {

			return RegStringToFundamental<_type>(raw, length, type, scanformat);
		}

		else return static_cast<_type>(0);
	}

	template<typename _type>
	typename std::enable_if<std::is_floating_point<_type>::value, _type>::type
	ConvertRegistryValue(void* raw, size_t length, DWORD type, const tchar_t* scanformat)
	{
		return static_cast<_type>(0);
	}


	// svctl::RegBinaryToFloat
	//
	// Converts a raw registry value into a floating point fundamental type
	//
	// Arguments:
	//
	//	raw		- Pointer to the raw registry value data
	//	length	- Length of the raw registry value data
	//	type	- Type/format of the raw registry value data

	template<typename _type>
	_type RegBinaryToFloat(void* raw, size_t length, DWORD)
	{
		// Use the length of the binary data to determine which floating point type to cast before conversion
		if(length >= sizeof(double)) return static_cast<_type>(*reinterpret_cast<double*>(raw));
		else if(length >= sizeof(float)) return static_cast<_type>(*reinterpret_cast<float*>(raw));
		else return static_cast<_type>(0);
	}

	// svctl::RegBinaryToInteger
	//
	// Converts a raw registry value into an integer fundamental type
	//
	// Arguments:
	//
	//	raw		- Pointer to the raw registry value data
	//	length	- Length of the raw registry value data
	//	type	- Type/format of the raw registry value data

	template<typename _type>
	_type RegBinaryToInteger(void* raw, size_t length, DWORD)
	{
		// Use the length of the binary data to determine which integer type to cast before conversion
		if(length >= sizeof(unsigned long long)) return static_cast<_type>(*reinterpret_cast<unsigned long long*>(raw));
		else if(length >= sizeof(unsigned long)) return static_cast<_type>(*reinterpret_cast<unsigned long*>(raw));
		else if(length >= sizeof(unsigned short)) return static_cast<_type>(*reinterpret_cast<unsigned short*>(raw));
		else if(length >= sizeof(unsigned char)) return static_cast<_type>(*reinterpret_cast<unsigned char*>(raw));
		else return static_cast<_type>(0);
	}

	template<typename _type>
	_type RegStringToBoolean(void* raw, size_t length, DWORD type)
	{
		(raw);
		(length);
		(type);
		// need static_assert for _type == bool
		return false;
	}

	// needs variadic format string
	template <typename _type, tchar_t _format>
	_type RegStringToFundamental(void* raw, size_t length, DWORD type)
	{
		_type value = static_cast<_type>(0);			// Assume failure --> zero

		// TODO : DEAL WITH REG_EXPAND_SZ FIRST
		(type);

		// Use sscanf_s to attempt to convert the string into the fundamental value type
		tchar_t format[3] = { _T('%'), _format , static_cast<tchar_t>(0) };
		_stscanf_s(reinterpret_cast<tchar_t*>(raw), format, &value, length);

		return value;
	}

	template <typename _type>
	_type RegStringFormat(void* raw, size_t length, DWORD type, const tchar_t* format)
	{
		_type value = static_cast<_type>(0);			// Assume failure --> zero

		// TODO : DEAL WITH REG_EXPAND_SZ FIRST
		(type);

		// Use sscanf_s to attempt to convert the string into the value type
		_stscanf_s(reinterpret_cast<tchar_t*>(raw), format, &value, length);

		return value;
	}

	template <typename _type> _type RegStringTest(void*, size_t, DWORD);

	template<> double RegStringTest<double>(void* raw, size_t length, DWORD type) { return RegStringFormat<double>(raw, length, type, _T("%d")); }

	//-------------------------------------------------------------------------
	// parameter::ReadFundamentalValue (private)
	//
	// Reads a fundamental data type parameter value from the registry
	//
	// Arguments:
	//
	//	NONE
		
	template <typename _type, parameter_convert<_type> _from_binary, parameter_convert<_type> _from_string>
	_type parameter::ReadFundamentalValue(void)
	{
		size_t				rawlength;			// Length of the raw registry value
		DWORD				rawtype;			// Type/format of the raw registry value
		
		_type value = static_cast<_type>(0);		// Resultant parameter value

		// Read the raw registry value, length and type information
		void* raw = ReadRawValue(&rawlength, &rawtype);
		if(!raw) return value;

		switch(rawtype) {

			// Binary / DoubleWord / QuadWord
			case REG_BINARY:
			case REG_DWORD:
			case REG_QWORD:
				value = _from_binary(raw, rawlength, rawtype);
				break;

			// String / ExpandString
			case REG_SZ:
			case REG_EXPAND_SZ:
				value = _from_string(raw, rawlength, rawtype);
				break;
		}

		FreeRawValue(raw);							// Release the allocated buffer
		return value;								// Return the copy of the value
	}

	/// NEW
	template <typename _type>
	_type parameter::ReadFundamental(const tchar_t* scanformat)
	{
		// Verify that this has been specified for a fundamental non-pointer data type only
		static_assert(std::is_fundamental<_type>::value == true, "parameter::ReadFundamental<> cannot be used with a non-fundamental data type");
		static_assert(std::is_pointer<_type>::value == false, "parameter::ReadFundamental<> cannot be used with a pointer data type");

		size_t				rawlength;				// Length of the raw registry value
		DWORD				rawtype;				// Type/format of the raw registry value
		
		_type value = static_cast<_type>(0);		// Resultant parameter value

		// Read the raw registry value, length and type information from the registry
		void* raw = ReadRawValue(&rawlength, &rawtype);
		if(!raw) return value;

		///
		if((rawtype == REG_BINARY) || (rawtype == REG_DWORD) || (rawtype == REG_QWORD)) {

			// INTEGER
			if(std::is_arithmetic<_type>::value) {

				if(rawlength >= sizeof(unsigned long long)) value = static_cast<_type>(*reinterpret_cast<unsigned long long*>(raw));
				else if(rawlength >= sizeof(unsigned long)) value = static_cast<_type>(*reinterpret_cast<unsigned long*>(raw));
				else if(rawlength >= sizeof(unsigned short)) value = static_cast<_type>(*reinterpret_cast<unsigned short*>(raw));
				else if(rawlength >= sizeof(unsigned char)) value = static_cast<_type>(*reinterpret_cast<unsigned char*>(raw));
			}

			// FLOATING POINT
			else if(std::is_floating_point<_type>::value) {

				if(rawlength >= sizeof(double)) value = static_cast<_type>(*reinterpret_cast<double*>(raw));
				else if(rawlength >= sizeof(float)) value = static_cast<_type>(*reinterpret_cast<float*>(raw));
			}

			// OTHER
			else if(rawlength >= sizeof(_type)) value = static_cast<_type>(*reinterpret_cast<_type*>(raw));
		}

		///
		else if(rawtype == REG_EXPAND_SZ) {

			DWORD required = ExpandEnvironmentStrings(reinterpret_cast<tchar_t*>(raw), nullptr, 0);
			tchar_t* temp = new tchar_t[required];
			if(temp) {

				DWORD result = ExpandEnvironmentStrings(reinterpret_cast<tchar_t*>(raw), temp, required);
				if(result) _stscanf_s(temp, scanformat, &value, result);
				delete[] temp;
			}

		}

		///
		else if(rawtype == REG_SZ) {

			_stscanf_s(reinterpret_cast<tchar_t*>(raw), scanformat, &value, rawlength);
		}

		FreeRawValue(raw);							// Release the allocated buffer
		return value;								// Return the copy of the value
	}

	// bool
	//
	template<> bool parameter::ReadValue<bool>(void) { return (ReadFundamentalValue<unsigned long, RegBinaryToInteger, RegStringToBoolean>() != 0); }

	// char / signed char / unsigned char
	//
	template<> char parameter::ReadValue<char>(void) { return ReadFundamentalValue<char, RegBinaryToInteger, RegStringToFundamental<char, 'c'>>(); }
	template<> signed char parameter::ReadValue<signed char>(void) { return ReadFundamentalValue<signed char, RegBinaryToInteger, RegStringToFundamental<signed char, 'i'>>(); }
	template<> unsigned char parameter::ReadValue<unsigned char>(void) { return ReadFundamentalValue<unsigned char, RegBinaryToInteger, RegStringToFundamental<unsigned char, 'u'>>(); }

	// __wchar_t / short / unsigned short
	//
	template<> __wchar_t parameter::ReadValue<__wchar_t>(void) { return ReadFundamentalValue<__wchar_t, RegBinaryToInteger, RegStringToFundamental<__wchar_t, 'c'>>(); }
	template<> short parameter::ReadValue<short>(void) { return ReadFundamentalValue<short, RegBinaryToInteger, RegStringToFundamental<short, 'i'>>(); }
	template<> unsigned short parameter::ReadValue<unsigned short>(void) { return ReadFundamentalValue<unsigned short, RegBinaryToInteger, RegStringToFundamental<unsigned short, 'u'>>(); }

	// int / long / unsigned int / unsigned long
	//
	template<> int parameter::ReadValue<int>(void) { return ReadFundamentalValue<int, RegBinaryToInteger, RegStringToFundamental<int, 'i'>>(); }
	template<> long parameter::ReadValue<long>(void)  { return ReadFundamentalValue<long, RegBinaryToInteger, RegStringToFundamental<long, 'i'>>(); }
	template<> unsigned int parameter::ReadValue<unsigned int>(void) { return ReadFundamentalValue<unsigned int, RegBinaryToInteger, RegStringToFundamental<unsigned int, 'u'>>(); }
	template<> unsigned long parameter::ReadValue<unsigned long>(void) { return ReadFundamentalValue<unsigned long, RegBinaryToInteger, RegStringToFundamental<unsigned long, 'u'>>(); }

	// long long / unsigned long long
	//
	template<> long long parameter::ReadValue<long long>(void) { return ReadFundamentalValue<long long, RegBinaryToInteger, RegStringToFundamental<long long, 'i'>>(); }
	template<> unsigned long long parameter::ReadValue<unsigned long long>(void) { return ReadFundamentalValue<unsigned long long, RegBinaryToInteger, RegStringToFundamental<unsigned long long, 'u'>>(); }

	// float / double / long double
	//template<> float parameter::ReadValue<float>(void) { return ReadFundamentalValue<float, RegBinaryToFloat, RegStringToFundamental<float, 'f'>>(); }
	//template<> double parameter::ReadValue<double>(void) { return ReadFundamentalValue<double, RegBinaryToFloat, RegStringTest>(); }
	//template<> long double parameter::ReadValue<long double>(void) { return ReadFundamentalValue<long double, RegBinaryToFloat, RegStringToFundamental<long double, 'd'>>(); }
	template<> float		parameter::ReadValue<float>(void)		{ return ReadFundamental<float>(_T("%f")); }
	template<> double		parameter::ReadValue<double>(void)		{ return ReadFundamental<double>(_T("%d")); }
	template<> long double	parameter::ReadValue<long double>(void)	{ return ReadFundamental<long double>(_T("%ld")); }

	// char* - don't do?

	// __wchar_t* - don't do ?

	// std::string

	// std::wstring

	// TODO: MULTI_SZ - vector? array?

};

#pragma warning(pop)
