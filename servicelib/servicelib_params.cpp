
#include "stdafx.h"
#include "servicelib.h"

#pragma warning(push, 4)

namespace svctl {

	//-------------------------------------------------------------------------
	// svctl::RegistryToBoolean
	//
	// Converts a registry_value instance into a boolean value
	//
	// Arguments:
	//
	//	value		- registry_value instance to convert

	static bool RegistryToBoolean(const registry_value& value)
	{
		// There needs to be some level of valid data in the registry value to continue
		if((value.Data == nullptr) || (value.Length == 0) || (value.Type == REG_NONE)) return false;

		// Binary data: TRUE if any bits are set
		if((value.Type == REG_BINARY) || (value.Type == REG_DWORD) || (value.Type == REG_QWORD)) {

			const uint8_t* data = reinterpret_cast<const uint8_t*>(value.Data);
			for(size_t index = 0; index < value.Length; index++) if(data[index]) return true;
			return false;
		}

		// String data: TRUE if the string reads "TRUE" (case-insensitive)
		else if((value.Type == REG_SZ) || (value.Type == REG_EXPAND_SZ))
			return (_tcsnicmp(reinterpret_cast<const tchar_t*>(value.Data), _T("true"), (value.Length / sizeof(TCHAR))) == 0);

		return false;
	}

	//-------------------------------------------------------------------------
	// svctl::RegistryToFloat
	//
	// Converts a registry_value instance into a floating point fundamental type
	//
	// Arguments:
	//
	//	value		- registry_value instance to convert
	//	scanformat	- sscanf() format string when converting from a string value

	template <typename _type, typename _scan_type = _type>
	static _type RegistryToFloat(const registry_value& value, const tchar_t* scanformat)
	{
		_type result = static_cast<_type>(0);

		// There needs to be some level of valid data in the registry value to continue
		if((value.Data == nullptr) || (value.Length == 0) || (value.Type == REG_NONE)) return result;

		// Binary data types
		if((value.Type == REG_BINARY) || (value.Type == REG_DWORD) || (value.Type == REG_QWORD)) {

			if(value.Length >= sizeof(double)) return static_cast<_type>(*reinterpret_cast<double*>(value.Data));
			else if(value.Length >= sizeof(float)) return static_cast<_type>(*reinterpret_cast<float*>(value.Data));
		}

		// String conversion types (EXPAND_SZ should be taken care of by registry_value)
		else if((value.Type == REG_SZ) || (value.Type == REG_EXPAND_SZ)) {

			_scan_type scanvalue = static_cast<_scan_type>(0);
			_stscanf_s(reinterpret_cast<tchar_t*>(value.Data), scanformat, &scanvalue, sizeof(scanvalue));
			result = static_cast<_type>(scanvalue);
		}

		return result;
	}

	//-------------------------------------------------------------------------
	// svctl::RegistryToIntegral
	//
	// Converts a registry_value instance into an integral fundamental type
	//
	// Arguments:
	//
	//	value		- registry_value instance to convert
	//	scanformat	- sscanf() format string when converting from a string value

	template <typename _type, typename _scan_type = _type>
	static _type RegistryToIntegral(const registry_value& value, const tchar_t* scanformat)
	{
		_type result = static_cast<_type>(0);

		// There needs to be some level of valid data in the registry value to continue
		if((value.Data == nullptr) || (value.Length == 0) || (value.Type == REG_NONE)) return result;

		// Binary data types
		if((value.Type == REG_BINARY) || (value.Type == REG_DWORD) || (value.Type == REG_QWORD)) {

			if(value.Length >= sizeof(unsigned long long)) return static_cast<_type>(*reinterpret_cast<unsigned long long*>(value.Data));
			else if(value.Length >= sizeof(unsigned long)) return static_cast<_type>(*reinterpret_cast<unsigned long*>(value.Data));
			else if(value.Length >= sizeof(unsigned short)) return static_cast<_type>(*reinterpret_cast<unsigned short*>(value.Data));
			else if(value.Length >= sizeof(unsigned char)) return static_cast<_type>(*reinterpret_cast<unsigned char*>(value.Data));
		}

		// String conversion types (EXPAND_SZ should be taken care of by registry_value)
		else if((value.Type == REG_SZ) || (value.Type == REG_EXPAND_SZ)) {

			_scan_type scanvalue = static_cast<_scan_type>(0);
			_stscanf_s(reinterpret_cast<tchar_t*>(value.Data), scanformat, &scanvalue, sizeof(scanvalue));
			result = static_cast<_type>(scanvalue);
		}

		return result;
	}

	//-------------------------------------------------------------------------
	// svctl::RegistryToString
	//
	// Converts a registry_value instance into a generic text string
	//
	// Arguments:
	//
	//	value		- registry_value instance to convert

	static tstring RegistryToString(const registry_value& value)
	{
		// TODO
		(value);
		return tstring();
	}

	//-------------------------------------------------------------------------
	// svctl::RegistryToStringVector
	//
	// Converts a registry_value instance into a vector of strings
	//
	// Arguments:
	//
	//	value		- registry_value instance to convert

	static std::vector<tstring> RegistryToStringVector(const registry_value& value)
	{
		// TODO
		(value);
		return std::vector<tstring>();
	}

	//-------------------------------------------------------------------------
	// registry_value::ConvertTo (generic)

	//
	// TODO - convert binary data only here -- no string conversions
	//

	//-------------------------------------------------------------------------
	// registry_value::ConvertTo (explicit specializations)

	// registry_value::ConvertTo<bool>
	template<> bool registry_value::ConvertTo<bool>(const registry_value& value) { return RegistryToBoolean(value); }

	// registry_value::ConvertTo<char | __wchar_t>
	// NOTE: String conversion format string differs between UNICODE and ANSI builds of the library
#ifdef _UNICODE
	template<> char registry_value::ConvertTo<char>(const registry_value& value) { return RegistryToIntegral<char>(value, _T("C")); }
	template<> __wchar_t registry_value::ConvertTo<__wchar_t>(const registry_value& value) { return RegistryToIntegral<__wchar_t>(value, _T("c")); }
#else
	template<> char registry_value::ConvertTo<char>(const registry_value& value) { return RegistryToIntegral<char>(value, _T("c")); }
	template<> __wchar_t registry_value::ConvertTo<__wchar_t>(const registry_value& value) { return RegistryToIntegral<__wchar_t>(value, _T("C")); }
#endif

	// registry_value::ConvertTo<signed char | unsigned char>
	// NOTE: String converstion requires casting from 16-bit value as Visual C++ does not support %hhi format string
	template<> signed char registry_value::ConvertTo<signed char>(const registry_value& value) { return RegistryToIntegral<signed char, short>(value, _T("%hi")); }
	template<> unsigned char registry_value::ConvertTo<unsigned char>(const registry_value& value) { return RegistryToIntegral<unsigned char, unsigned short>(value, _T("%hi")); }

	// registry_value::ConvertTo<short | unsigned short>
	template<> short registry_value::ConvertTo<short>(const registry_value& value) { return RegistryToIntegral<short>(value, _T("%hi")); }
	template<> unsigned short registry_value::ConvertTo<unsigned short>(const registry_value& value) { return RegistryToIntegral<unsigned short>(value, _T("%hi")); }
	
	// registry_value::ConvertTo<int | unsigned int>
	template<> int registry_value::ConvertTo<int>(const registry_value& value) { return RegistryToIntegral<int>(value, _T("%i")); }
	template<> unsigned int registry_value::ConvertTo<unsigned int>(const registry_value& value) { return RegistryToIntegral<unsigned int>(value, _T("%i")); }
	
	// registry_value::ConvertTo<long | unsigned long>
	template<> long registry_value::ConvertTo<long>(const registry_value& value) { return RegistryToIntegral<long>(value, _T("%li")); }
	template<> unsigned long registry_value::ConvertTo<unsigned long>(const registry_value& value) { return RegistryToIntegral<unsigned long>(value, _T("%li")); }

	// registry_value::ConvertTo<long long | unsigned long long>
	template<> long long registry_value::ConvertTo<long long>(const registry_value& value) { return RegistryToIntegral<long long>(value, _T("%lli")); }
	template<> unsigned long long registry_value::ConvertTo<unsigned long long>(const registry_value& value) { return RegistryToIntegral<unsigned long long>(value, _T("%lli")); }
	
	// registry_value::ConvertTo<float | double | long double>
	template<> float registry_value::ConvertTo<float>(const registry_value& value) { return RegistryToFloat<float>(value, _T("%f")); }
	template<> double registry_value::ConvertTo<double>(const registry_value& value) { return RegistryToFloat<double>(value, _T("%d")); }
	template<> long double registry_value::ConvertTo<long double>(const registry_value& value) { return RegistryToFloat<long double>(value, _T("%ld")); }

	// tstring
	// TODO

	// vector<tstring>
	// TODO
};


#pragma warning(pop)
