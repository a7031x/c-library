/*
	Author:	HouSD
	Date:	2013/02/27
*/

#pragma once
#include <boost/lexical_cast.hpp>
#include <custom/codepage.hpp>
#include <boost/mpl/if.hpp>
#include <string>
#include <fstream>
#include <DbgHelp.h>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#pragma comment(lib, "DbgHelp.lib")

#define	ooo	debug::od();
#define	mmm	debug::md();
namespace debug
{
	template<typename ValueType>
	inline std::wstring text_cast(ValueType value)
	{
		struct code_change
		{
			std::wstring operator()(const std::string& value)
			{
				return codepage::acp_to_unicode(value);
			}
		};
		struct lexical_change
		{
			std::wstring operator()(ValueType value)
			{
				return boost::lexical_cast<std::wstring, ValueType>(value);
			}
		};
		return boost::mpl::if_c<std::is_convertible<ValueType, std::string>::value, code_change, lexical_change>::type()(value);
	}
	template<typename T1, typename... T2>
	inline std::wstring text_cast(const T1& t1, const T2&... t2)
	{
		return text_cast(t1) + L" " + text_cast(t2...);
	}

	template<typename... ValueType>
	inline void md(const ValueType&... value)
	{
		md(text_cast(value...));
	}

	template<typename... ValueType>
	inline void od(const ValueType&... value)
	{
		od(text_cast(value...));
	}

	template<typename... ValueType>
	inline void fd(const ValueType&... value)
	{
		fd(text_cast(value...));
	}

	inline void md(const std::wstring& text)
	{
		MessageBoxW(nullptr, text.c_str(), nullptr, MB_OK);
	}
	inline void od(const std::wstring& text)
	{
		OutputDebugStringW((text + L"\n").c_str());
	}
	inline void fd(const std::wstring& text)
	{
		static boost::recursive_mutex mutex;
		size_t referenceAddress = (size_t)&mutex;
		static std::ofstream file = [referenceAddress]()->std::ofstream
		{
			wchar_t path[MAX_PATH];
			SymInitialize(GetCurrentProcess(), nullptr, true);
			auto base = SymGetModuleBase(GetCurrentProcess(), referenceAddress);
			GetModuleFileNameW((HMODULE)base, path, _countof(path));
			std::ofstream fs(std::wstring(path) + L".txt", std::ios::binary);
			SymCleanup(GetCurrentProcess());
			char bom[2] = {-1, -2};
			fs.write(bom, sizeof(bom));
			return fs;
		}();
		boost::lock_guard<decltype(mutex)> lock(mutex);
		file.write(reinterpret_cast<const char*>(text.c_str()), text.size() * sizeof(wchar_t));
		file.write(reinterpret_cast<const char*>(L"\r\n"), 4);
		od(text);
	}
	inline void od()
	{
		static int k = 0;
		od(L"debugutility: " + text_cast(++k));
	}
	inline void md()
	{
		static int k = 0;
		md(L"debugutility: " + text_cast(++k));
	}
};
using namespace debug;