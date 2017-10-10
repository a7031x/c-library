#include <Windows.h>
#include <Shlwapi.h>
#include <DbgHelp.h>
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "dbghelp.lib")

inline void enable_dump()
{
	auto unhandled_handler = [](EXCEPTION_POINTERS* e)->long
	{
		wchar_t path[MAX_PATH];
		wchar_t procName[MAX_PATH];
		GetModuleFileNameW(nullptr, path, _countof(path));
		wcscpy_s(procName, PathFindFileNameW(path));
		PathRemoveFileSpecW(path);
		PathCombineW(path, path, L"minidump");
		BOOL result = CreateDirectoryW(path, nullptr);
		if(FALSE == result && ERROR_ALREADY_EXISTS != GetLastError())
		{
			GetTempPathW(_countof(path), path);
			PathCombineW(path, path, L"minidump");
			CreateDirectoryW(path, nullptr);
		}
		SYSTEMTIME time;
		GetLocalTime(&time);
		wchar_t filename[MAX_PATH];
		wsprintfW(filename, L"%4d-%02d-%02d-%02d-%02d-%02d_%s.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, procName);
		PathCombineW(path, path, filename);
		auto file = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (INVALID_HANDLE_VALUE != file)
		{
			MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
			exceptionInfo.ThreadId = GetCurrentThreadId();
			exceptionInfo.ExceptionPointers = e;
			exceptionInfo.ClientPointers = FALSE;
			MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file,
				MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory), nullptr != e ? &exceptionInfo : nullptr, nullptr, nullptr);
			CloseHandle(file);
		}
		return EXCEPTION_CONTINUE_SEARCH;
	};
	SetUnhandledExceptionFilter(unhandled_handler);
}