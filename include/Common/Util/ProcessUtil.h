#pragma once

#ifdef _WIN32
	#include <windows.h>
	#include <tlhelp32.h>
#else

#endif

#include <Common/Util/StringUtil.h>

class ProcessUtil
{
public:
	static intptr_t CreateProc(const std::string& command)
	{
	#ifdef _WIN32
		// Allocate STARTUPINFO and PROCESS_INFORMATION objects
		STARTUPINFO si;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
    
		PROCESS_INFORMATION pi;
		ZeroMemory(&pi, sizeof(pi));
    
		// Create the process
		if (!CreateProcess(NULL, (LPTSTR)StringUtil::ToWide(command).c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		{
			//wchar_t buf[256];
			//FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			//	NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			//	buf, (sizeof(buf) / sizeof(wchar_t)), NULL);
			//std::wstring lastError(buf);
			//std::string lastErrorStr(lastError.begin(), lastError.end());
			return 0;
		}

		return (intptr_t)pi.hProcess;

	#else
		pid_t processId;
		if ((processId = fork()) == 0)
		{
			//char app[] = "/bin/echo";
			std::string app = command;
			char* appArray = new char[app.size() + 1];
			std::copy(app.begin(), app.end(), appArray);
			appArray[app.size()] = '\0';
        
			std::string success = "success";
			char* successArray = new char[success.size() + 1];
			std::copy(success.begin(), success.end(), successArray);
			successArray[success.size()] = '\0';
        
			char* const argv[] = { appArray, successArray, NULL };
			if (execv(app.c_str(), argv) < 0)
			{
				perror("execv error");
			}
        
			delete[] successArray;
			delete[] appArray;
		}

		return (long)processId;
	#endif
	}

	static bool KillProc(const long processId)
	{
		// Wait for the process to finish
		//WaitForSingleObject(pi.hProcess, INFINITE);
		//CloseHandle(pi.hProcess);
		//CloseHandle(pi.hThread);

		return false;
	}

	static long GetProcessId(const std::string processName)
	{
#ifdef _WIN32
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(PROCESSENTRY32);

		const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		if (!Process32First(snapshot, &entry))
		{
			CloseHandle(snapshot);
			return 0;
		}

		do {
			if (!wcscmp(entry.szExeFile, StringUtil::ToWide(processName).c_str()))
			{
				CloseHandle(snapshot);
				return entry.th32ProcessID;
			}
		} while (Process32Next(snapshot, &entry));

		CloseHandle(snapshot);
#else

#endif
		return 0;
	}
};