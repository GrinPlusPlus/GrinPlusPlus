#include "ShutdownManagerImpl.h"

#include <Infrastructure/ShutdownManager.h>
#include <Infrastructure/Logger.h>
#include <csignal>

ShutdownManager& ShutdownManager::GetInstance()
{
	static ShutdownManager instance;
	return instance;
}

static void SigIntHandler(int signum)
{
	printf("\n\n%d signal received\n\n", signum);
	ShutdownManager::GetInstance().Shutdown();
}

namespace ShutdownManagerAPI
{
	SHUTDOWN_MANAGER_API void RegisterHandlers()
	{
		signal(SIGINT, SigIntHandler);
		signal(SIGTERM, SigIntHandler);
		signal(SIGABRT, SigIntHandler);
		signal(9, SigIntHandler);
	}

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