#pragma once

#include <string>
#include <thread>
#include <ImportExport.h>

#ifdef MW_INFRASTRUCTURE
#define THREAD_MANAGER_API __declspec(dllexport)
#else
#define THREAD_MANAGER_API __declspec(dllimport)
#endif

namespace ThreadManagerAPI
{
	// Future: Implement a CreateThread method that takes the name, function, and parameters.
	THREAD_MANAGER_API std::string GetCurrentThreadName();
	THREAD_MANAGER_API void SetThreadName(const std::thread::id& threadId, const std::string& threadName);
	THREAD_MANAGER_API void SetCurrentThreadName(const std::string& threadName);
};