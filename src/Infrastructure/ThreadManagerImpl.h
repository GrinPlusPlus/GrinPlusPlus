#pragma once

#include <string>
#include <thread>
#include <unordered_map>
#include <shared_mutex>

class ThreadManager
{
public:
	static ThreadManager& GetInstance();

	// Future: Implement a CreateThread method that takes the name, function, and parameters.
	std::string GetCurrentThreadName() const;
	void SetThreadName(const std::thread::id& threadId, const std::string& threadName);
	void SetCurrentThreadName(const std::string& threadName);

private:
	mutable std::shared_mutex m_threadNamesMutex;
	std::unordered_map<std::thread::id, std::string> m_threadNamesById;
};