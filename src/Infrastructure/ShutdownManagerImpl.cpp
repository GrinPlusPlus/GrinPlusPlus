#include "ShutdownManagerImpl.h"

#include <Infrastructure/ShutdownManager.h>
#include <Infrastructure/Logger.h>

ShutdownManager& ShutdownManager::GetInstance()
{
	static ShutdownManager instance;
	return instance;
}

namespace ShutdownManagerAPI
{
	SHUTDOWN_MANAGER_API const std::atomic_bool& WasShutdownRequested()
	{
		return ShutdownManager::GetInstance().WasShutdownRequested();
	}

	SHUTDOWN_MANAGER_API void Shutdown()
	{
		LOG_INFO("Shutdown requested");
		ShutdownManager::GetInstance().Shutdown();
	}
};