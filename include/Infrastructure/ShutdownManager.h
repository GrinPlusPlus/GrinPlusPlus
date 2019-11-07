#pragma once

#include <atomic>
#include <Common/ImportExport.h>

#ifdef MW_INFRASTRUCTURE
#define SHUTDOWN_MANAGER_API EXPORT
#else
#define SHUTDOWN_MANAGER_API IMPORT
#endif

namespace ShutdownManagerAPI
{
	SHUTDOWN_MANAGER_API const std::atomic_bool& WasShutdownRequested();
	SHUTDOWN_MANAGER_API void Shutdown();
};
