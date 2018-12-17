#include "ThreadManagerImpl.h"

#include <Infrastructure/ThreadManager.h>
#include <sstream>

ThreadManager& ThreadManager::GetInstance()
{
	static ThreadManager threadManager;
	return threadManager;
}

std::string ThreadManager::GetCurrentThreadName() const
{
	std::shared_lock<std::shared_mutex> readLock(m_threadNamesMutex);

	std::thread::id threadId = std::this_thread::get_id();

	if (m_threadNamesById.find(threadId) != m_threadNamesById.end())
	{
		return m_threadNamesById.find(threadId)->second;
	}

	std::stringstream ss;
	ss << "THREAD(ID:" << threadId << ")";
	return ss.str();
}

void ThreadManager::SetThreadName(const std::thread::id& threadId, const std::string& threadName)
{
	std::unique_lock<std::shared_mutex> lockGuard(m_threadNamesMutex);

	std::stringstream ss;
	ss << threadName << "(ID:" << threadId << ")";
	m_threadNamesById[threadId] = ss.str();
}

void ThreadManager::SetCurrentThreadName(const std::string& threadName)
{
	std::unique_lock<std::shared_mutex> lockGuard(m_threadNamesMutex);

	std::stringstream ss;
	ss << threadName << "(ID:" << std::this_thread::get_id() << ")";
	m_threadNamesById[std::this_thread::get_id()] = ss.str();
}

namespace ThreadManagerAPI
{
	// Future: Implement a CreateThread method that takes the name, function, and parameters.
	THREAD_MANAGER_API std::string GetCurrentThreadName()
	{
		return ThreadManager::GetInstance().GetCurrentThreadName();
	}

	THREAD_MANAGER_API void SetThreadName(const std::thread::id& threadId, const std::string& threadName)
	{
		ThreadManager::GetInstance().SetThreadName(threadId, threadName);
	}

	THREAD_MANAGER_API void SetCurrentThreadName(const std::string& threadName)
	{
		ThreadManager::GetInstance().SetCurrentThreadName(threadName);
	}
};