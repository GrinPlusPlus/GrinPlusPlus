#pragma once

#include <Common/ImportExport.h>
#include <string>
#include <thread>

#ifdef MW_INFRASTRUCTURE
#define THREAD_MANAGER_API EXPORT
#else
#define THREAD_MANAGER_API IMPORT
#endif

namespace ThreadManagerAPI
{
// Future: Implement a CreateThread method that takes the name, function, and parameters.

//
// Retrieves the name of the current thread.
//
THREAD_MANAGER_API std::string GetCurrentThreadName();

//
// Set the name of the thread with the thread id.
//
THREAD_MANAGER_API void SetThreadName(const std::thread::id &threadId, const std::string &threadName);

//
// Set the name of the current thread.
//
THREAD_MANAGER_API void SetCurrentThreadName(const std::string &threadName);
}; // namespace ThreadManagerAPI
