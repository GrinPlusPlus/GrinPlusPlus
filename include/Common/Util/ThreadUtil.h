#pragma once

#include <Common/Util/TimeUtil.h>
#include <chrono>
#include <atomic>
#include <thread>
#include <vector>

class ThreadUtil
{
public:
	//
	// Sleeps for the given number of milliseconds. Checks bool in given interval, and breaks early if the passed in bool gets set to true.
	//
	static void SleepFor(const std::chrono::milliseconds& millisToSleep, const std::chrono::milliseconds& checkInterval, const std::atomic_bool& terminate)
	{
		std::chrono::time_point wakeTime = std::chrono::system_clock::now() + millisToSleep;
		while (!terminate)
		{
			if (std::chrono::system_clock::now() >= wakeTime)
			{
				break;
			}

			std::this_thread::sleep_for(checkInterval);
		}
	}

	//
	// Sleeps for the given duration. Checks bool every 5ms, and breaks early if it gets set to true.
	//
	template<class _Rep, class _Period> inline
	static void SleepFor(const std::chrono::duration<_Rep, _Period>& duration, const std::atomic_bool& terminate)
	{
		return SleepFor(
			std::chrono::duration_cast<std::chrono::milliseconds>(duration),
			std::chrono::milliseconds(5),
			terminate
		);
	}

	static void Join(std::thread& thread)
	{
		try
		{
			if (thread.joinable())
			{
				thread.join();
			}
		}
		catch (...)
		{

		}
	}

	static void JoinAll(std::vector<std::thread>& threads)
	{
		for (auto& thread : threads)
		{
			if (thread.joinable())
			{
				thread.join();
			}
		}
	}

	static void Detach(std::thread& thread)
	{
		if (thread.joinable())
		{
			thread.detach();
		}
	}
};