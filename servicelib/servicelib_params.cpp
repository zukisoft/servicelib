
#include "stdafx.h"
#include "servicelib.h"

#pragma warning(push, 4)

namespace svctl {

	//-------------------------------------------------------------------------
	// registry_value::RegistryToBoolean (private, static)
	//
	// Converts a registry_value instance into a boolean value
	//
	// Arguments:
	//
	//	value		- registry_value instance to convert

	bool registry_value::RegistryToBoolean(const registry_value& value)
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
		else if((value.Type == REG_SZ) || (value.Type == REG_EXPAND_SZ) || (value.Type == REG_MULTI_SZ))
			return (_tcsnicmp(reinterpret_cast<const tchar_t*>(value.Data), _T("true"), (value.Length / sizeof(tchar_t))) == 0);

		return false;
	}

	//-------------------------------------------------------------------------
	// registry_value::RegistryToFloat (private, static)
	//
	// Converts a registry_value instance into a floating point fundamental type
	//
	// Arguments:
	//
	//	value		- registry_value instance to convert
	//	scanformat	- sscanf() format string when converting from a string value

	template <typename _type, typename _scan_type>
	_type registry_value::RegistryToFloat(const registry_value& value, const tchar_t* scanformat)
	{
		_type result = static_cast<_type>(0);

		// There needs to be some level of valid data in the registry value to continue
		if((value.Data == nullptr) || (value.Length == 0) || (value.Type == REG_NONE)) return result;

		// Binary data types
		if((value.Type == REG_BINARY) || (value.Type == REG_DWORD) || (value.Type == REG_QWORD)) {

			if(value.Length >= sizeof(double)) return static_cast<_type>(*reinterpret_cast<double*>(value.Data));
			else if(value.Length >= sizeof(float)) return static_cast<_type>(*reinterpret_cast<float*>(value.Data));
		}

		// String conversion types (EXPAND_SZ should be taken care of by registry_value; MULTI_SZ will
		// only process the first string in the array)
		else if((value.Type == REG_SZ) || (value.Type == REG_EXPAND_SZ) || (value.Type == REG_MULTI_SZ)) {

			_scan_type scanvalue = static_cast<_scan_type>(0);
			_stscanf_s(reinterpret_cast<tchar_t*>(value.Data), scanformat, &scanvalue, sizeof(scanvalue));
			result = static_cast<_type>(scanvalue);
		}

		return result;
	}

	//-------------------------------------------------------------------------
	// registry_value::RegistryToIntegral (private, static)
	//
	// Converts a registry_value instance into an integral fundamental type
	//
	// Arguments:
	//
	//	value		- registry_value instance to convert
	//	scanformat	- sscanf() format string when converting from a string value

	template <typename _type, typename _scan_type>
	_type registry_value::RegistryToIntegral(const registry_value& value, const tchar_t* scanformat)
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

		// String conversion types (EXPAND_SZ should be taken care of by registry_value; MULTI_SZ will
		// only process the first string in the array)
		else if((value.Type == REG_SZ) || (value.Type == REG_EXPAND_SZ) || (value.Type == REG_MULTI_SZ)) {

			_scan_type scanvalue = static_cast<_scan_type>(0);
			_stscanf_s(reinterpret_cast<tchar_t*>(value.Data), scanformat, &scanvalue, sizeof(scanvalue));
			result = static_cast<_type>(scanvalue);
		}

		return result;
	}

	//-------------------------------------------------------------------------
	// registry_value::RegistryToString (private, static)
	//
	// Converts a registry_value instance into an ANSI std::string instance
	//
	// Arguments:
	//
	//	value		- registry_value instance to convert

	template<> std::string registry_value::RegistryToString<std::string>(const registry_value& value)
	{
		// There needs to be some level of valid data in the registry value to continue
		if((value.Data == nullptr) || (value.Length == 0) || (value.Type == REG_NONE)) return std::string();

		// REG_DWORD
		if(value.Type == REG_DWORD) return std::to_string(*reinterpret_cast<unsigned long*>(value.Data));

		// REG_QWORD
		else if(value.Type == REG_QWORD) return std::to_string(*reinterpret_cast<unsigned long long*>(value.Data));

		// String data types (EXPAND_SZ should be taken care of by registry_value; MULTI_SZ will
		// only process the first string in the array)
		else if((value.Type == REG_SZ) || (value.Type == REG_EXPAND_SZ) || (value.Type == REG_MULTI_SZ)) {

#ifdef _UNICODE
			// TODO: watch for missing NULL terminators - use +1 on the vector<> and specify lengths
			// UNICODE --> Convert string into ANSI
			int required = WideCharToMultiByte(CP_ACP, 0, reinterpret_cast<const wchar_t*>(value.Data), -1, nullptr, 0, nullptr, nullptr);
			if(required) {

				std::vector<char> buffer(required);
				WideCharToMultiByte(CP_ACP, 0, reinterpret_cast<const wchar_t*>(value.Data), -1, buffer.data(), required, nullptr, nullptr);
				return std::string(buffer.data());
			}
#else
			// ANSI --> No conversion necessary
			return std::string(reinterpret_cast<const char*>(value.Data), (value.Length / sizeof(char)));
#endif
		}

		return std::string();
	}

	//-------------------------------------------------------------------------
	// registry_value::RegistryToString (private, static)
	//
	// Converts a registry_value instance into a UNICODE std::wstring instance
	//
	// Arguments:
	//
	//	value		- registry_value instance to convert

	template<> std::wstring registry_value::RegistryToString<std::wstring>(const registry_value& value)
	{
		// There needs to be some level of valid data in the registry value to continue
		if((value.Data == nullptr) || (value.Length == 0) || (value.Type == REG_NONE)) return std::wstring();

		// REG_DWORD
		if(value.Type == REG_DWORD) return std::to_wstring(*reinterpret_cast<unsigned long*>(value.Data));

		// REG_QWORD
		else if(value.Type == REG_QWORD) return std::to_wstring(*reinterpret_cast<unsigned long long*>(value.Data));

		// String data types (EXPAND_SZ should be taken care of by registry_value; MULTI_SZ will
		// only process the first string in the array)
		else if((value.Type == REG_SZ) || (value.Type == REG_EXPAND_SZ) || (value.Type == REG_MULTI_SZ)) {

#ifdef _UNICODE
			// UNICODE --> No conversion necessary
			return std::wstring(reinterpret_cast<const wchar_t*>(value.Data), (value.Length / sizeof(wchar_t)));
#else
			// TODO: watch for missing NULL terminators - use +1 on the vector<> and specify lengths

			// ANSI --> Convert the string into UNICODE
			int required = MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<const char*>(value.Data), -1, nullptr, 0);
			if(required) {

				std::vector<wchar_t> buffer(required);
				MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<const char*>(value.Data), -1, buffer.data(), required);
				return std::wstring(buffer.data());
			}
#endif
		}

		return std::wstring();
	}

	//-------------------------------------------------------------------------
	// registry_value::RegistryToStringVector (private, static)
	//
	// Converts a registry_value instance into a vector of ANSI strings
	//
	// Arguments:
	//
	//	value		- registry_value instance to convert

	template<> std::vector<std::string> registry_value::RegistryToStringVector<std::string>(const registry_value& value)
	{
		// There needs to be some level of valid data in the registry value to continue
		if((value.Data == nullptr) || (value.Length == 0) || (value.Type == REG_NONE)) return std::vector<std::string>();

		// If the data type is not REG_MULTI_SZ, return a vector with just the one converted string in it
		if(value.Type != REG_MULTI_SZ) return std::vector<std::string> { RegistryToString<std::string>(value) };

		// TODO: Continue here

		return std::vector<std::string>();
	}

	//-------------------------------------------------------------------------
	// registry_value::RegistryToStringVector (private, static)
	//
	// Converts a registry_value instance into a vector of UNICODE strings
	//
	// Arguments:
	//
	//	value		- registry_value instance to convert

	template<> std::vector<std::wstring> registry_value::RegistryToStringVector<std::wstring>(const registry_value& value)
	{
		// There needs to be some level of valid data in the registry value to continue
		if((value.Data == nullptr) || (value.Length == 0) || (value.Type == REG_NONE)) return std::vector<std::wstring>();

		// If the data type is not REG_MULTI_SZ, return a vector with just the one converted string in it
		if(value.Type != REG_MULTI_SZ) return std::vector<std::wstring> { RegistryToString<std::wstring>(value) };

		// TODO: Continue here

		return std::vector<std::wstring>();
	}

	//-------------------------------------------------------------------------
	// registry_value::ConvertTo<>
	//
	// Explicit Specializations

	//
	// BOOLEAN SPECIALIZATIONS
	//

	// registry_value::ConvertTo<bool>
	template<> bool registry_value::ConvertTo<bool>(const registry_value& value) { return RegistryToBoolean(value); }

	//
	// CHARACTER SPECIALIZATIONS
	//

	// registry_value::ConvertTo<char>
#ifdef _UNICODE
	template<> char registry_value::ConvertTo<char>(const registry_value& value) { return RegistryToIntegral<char>(value, _T("C")); }
#else
	template<> char registry_value::ConvertTo<char>(const registry_value& value) { return RegistryToIntegral<char>(value, _T("c")); }
#endif

	// registry_value::ConvertTo<__wchar_t>
#ifdef _UNICODE
	template<> __wchar_t registry_value::ConvertTo<__wchar_t>(const registry_value& value) { return RegistryToIntegral<__wchar_t>(value, _T("c")); }
#else
	template<> __wchar_t registry_value::ConvertTo<__wchar_t>(const registry_value& value) { return RegistryToIntegral<__wchar_t>(value, _T("C")); }
#endif

	//
	// INTEGRAL SPECIALIZATIONS
	//

	// registry_value::ConvertTo<signed char>
	//
	// String conversion format: short
	template<> signed char registry_value::ConvertTo<signed char>(const registry_value& value) { return RegistryToIntegral<signed char, short>(value, _T("%hi")); }

	// registry_value::ConvertTo<unsigned char>
	//
	// String scan format: short
	template<> unsigned char registry_value::ConvertTo<unsigned char>(const registry_value& value) { return RegistryToIntegral<unsigned char, short>(value, _T("%hi")); }

	// registry_value::ConvertTo<short>
	template<> short registry_value::ConvertTo<short>(const registry_value& value) { return RegistryToIntegral<short>(value, _T("%hi")); }

	// registry_value::ConvertTo<unsigned short>
	//
	// String conversion format --> short
	template<> unsigned short registry_value::ConvertTo<unsigned short>(const registry_value& value) { return RegistryToIntegral<unsigned short, short>(value, _T("%hi")); }
	
	// registry_value::ConvertTo<int>
	template<> int registry_value::ConvertTo<int>(const registry_value& value) { return RegistryToIntegral<int>(value, _T("%i")); }

	// registry_value::ConvertTo<unsigned int>
	//
	// String conversion format --> int
	template<> unsigned int registry_value::ConvertTo<unsigned int>(const registry_value& value) { return RegistryToIntegral<unsigned int, int>(value, _T("%i")); }
	
	// registry_value::ConvertTo<long>
	template<> long registry_value::ConvertTo<long>(const registry_value& value) { return RegistryToIntegral<long>(value, _T("%li")); }

	// registry_value::ConvertTo<unsigned long>
	//
	// String conversion format --> long
	template<> unsigned long registry_value::ConvertTo<unsigned long>(const registry_value& value) { return RegistryToIntegral<unsigned long, long>(value, _T("%li")); }

	// registry_value::ConvertTo<long long>
	template<> long long registry_value::ConvertTo<long long>(const registry_value& value) { return RegistryToIntegral<long long>(value, _T("%lli")); }

	// registry_value::ConvertTo<unsigned long long>
	//
	// String conversion format --> long long
	template<> unsigned long long registry_value::ConvertTo<unsigned long long>(const registry_value& value) { return RegistryToIntegral<unsigned long long, long long>(value, _T("%lli")); }

	//
	// FLOATING POINT SPECIALIZATIONS
	//
	
	// registry_value::ConvertTo<float>
	template<> float registry_value::ConvertTo<float>(const registry_value& value) { return RegistryToFloat<float>(value, _T("%f")); }

	// registry_value::ConvertTo<double>
	template<> double registry_value::ConvertTo<double>(const registry_value& value) { return RegistryToFloat<double>(value, _T("%d")); }

	// registry_value::ConvertTo<long double>
	template<> long double registry_value::ConvertTo<long double>(const registry_value& value) { return RegistryToFloat<long double>(value, _T("%ld")); }

	//
	// STRING SPECIALIZATIONS
	//

	// registry_value::ConvertTo<std::string>
	template<> std::string registry_value::ConvertTo<std::string>(const registry_value& value) { return RegistryToString<std::string>(value); }

	// registry_value::ConvertTo<std::wstring>
	template<> std::wstring registry_value::ConvertTo<std::wstring>(const registry_value& value) { return RegistryToString<std::wstring>(value); }

	//
	// STRING ARRAY SPECIALIZATIONS
	//

	// registry_value::ConvertTo<std::vector<std::string>>
	template<> std::vector<std::string> registry_value::ConvertTo<std::vector<std::string>>(const registry_value& value) { return RegistryToStringVector<std::string>(value); }

	// registry_value::ConvertTo<std::vector<std::wstring>>
	template<> std::vector<std::wstring> registry_value::ConvertTo<std::vector<std::wstring>>(const registry_value& value) { return RegistryToStringVector<std::wstring>(value); }
};


#pragma warning(pop)
