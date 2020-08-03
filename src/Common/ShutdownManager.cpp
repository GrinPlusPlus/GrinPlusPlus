#include <Infrastructure/ShutdownManager.h>
#include <Infrastructure/Logger.h>
#include <csignal>
#include <atomic>

class ShutdownManager
{
public:
	static ShutdownManager& GetInstance();

	inline const std::atomic_bool& WasShutdownRequested() const { return m_shutdownRequested; }
	inline void Shutdown() { m_shutdownRequested = true; }

private:
	mutable std::atomic_bool m_shutdownRequested = { false };
};

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