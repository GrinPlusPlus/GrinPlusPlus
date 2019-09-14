#pragma once

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