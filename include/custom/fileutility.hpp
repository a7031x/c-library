/*
	Author:	HouSD
	Date:	2013/02/27

	This library encapsulates the file operations and exposes handy way to read/write/create a file.
*/

#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <custom/runtime_context.hpp>
#include <shlobj.h>
#include <filesystem>
#pragma comment(lib, "Shell32.lib")

#ifdef _DEBUG
#pragma comment(linker, "/nodefaultlib:libcmt.lib")
#endif // !DEBUG

//The handy micro to take benefit of right reference to improve performance.

namespace fileutility
{
	inline wstring get_full_path(const std::experimental::filesystem::path& name)
	{
		if(boost::algorithm::contains(name.wstring(), L":") || name.empty()) return name.wstring();
		return boost::algorithm::replace_all_copy(std::experimental::filesystem::absolute(name).wstring(), L"/", L"\\");
	}

	inline wstring get_normalized_path(const std::experimental::filesystem::path& name)
	{
		auto npath = get_full_path(name);
		boost::replace_all(npath, L"\\", L"/");
		if(boost::iends_with(npath, L"/"))
			npath.pop_back();
		return npath;
	}
	inline wstring get_parent_path(const std::experimental::filesystem::path& path)
	{
		auto npath = std::experimental::filesystem::path(get_normalized_path(path)).parent_path();
		auto wpath = npath.wstring();
		if(boost::ends_with(wpath, L"/"))
			wpath.pop_back();
		return wpath;
	}
	inline wstring get_real_path(const std::experimental::filesystem::path& name)
	{ 
		HRESULT			result;
		WCHAR			temp_path[MAX_PATH * 4] = {0};
		wstring			real_path = get_normalized_path(name);
		IShellLinkW*	psl;
		const wchar_t*	key = L"8D411CEB-17C2-4DEF-AA1B-6CB8B93C41F8";

		if(runtime_context::thread::exists(key) == false)
		{
			runtime_context::thread::value(key) = 1;
			CoInitialize(nullptr);
		}
		result = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl); 
		if(S_OK == result)
		{
			IPersistFile*   ppf;
			result = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf); 
			if(S_OK == result) 
			{ 
				result = ppf->Load(real_path.c_str(), 0);
				if(S_OK == result)
				{
					result = psl->GetPath(temp_path, sizeof(temp_path) / sizeof(WCHAR), NULL, SLGP_UNCPRIORITY);
					if(S_OK == result && temp_path[0])
						real_path = temp_path;
				}
				ppf->Release(); 
			}
			psl->Release();
		}
		return get_normalized_path(real_path);
	}
	inline uint64_t file_size(const std::experimental::filesystem::path& path)
	{
		if(std::experimental::filesystem::is_regular_file(path))
			return std::experimental::filesystem::file_size(path);
		else
			return 0;
	}

	inline vector<char> read_file(const std::experimental::filesystem::path& p)
	{
		ifstream file(p.generic_wstring(), ios::binary);
		auto size = fileutility::file_size(p);
		vector<char> buffer((size_t)size);
		if(0 < size)
			file.read(&buffer[0], size);
		file.close();
		return buffer;
	}

	inline void write_file(const std::experimental::filesystem::path& p, const vector<char>& buffer)
	{
		std::error_code ec;
		std::experimental::filesystem::create_directories(std::experimental::filesystem::path(p).parent_path(), ec);

		ofstream file(p.generic_wstring(), ofstream::trunc | ios::binary);
		if(0 < buffer.size())
			file.write(&buffer[0], buffer.size());
		file.close();
	}
	inline wstring remove_extension(const std::experimental::filesystem::path& name)
	{
		auto rem = name.wstring();
		auto pos = boost::find_last(rem, L".");
		return wstring(rem.begin(), pos.begin());
	}
	inline wstring remove_extensions(const std::experimental::filesystem::path& name)
	{
		auto rem = name.wstring();
		auto pos = boost::find_first(rem, L".");
		return wstring(rem.begin(), pos.begin());
	}
	inline 	wstring relative_path(const std::experimental::filesystem::path& folder, const std::experimental::filesystem::path& path)
	{
		auto base = fileutility::get_normalized_path(folder) + L"/";
		auto npath = fileutility::get_normalized_path(path);
		if(boost::istarts_with(npath, base))
			return npath.substr(base.size());
		else
			return npath;
	}
}