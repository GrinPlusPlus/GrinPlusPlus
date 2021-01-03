#pragma once

#include <Common/Util/TimeUtil.h>
#include <chrono>
#include <atomic>
#include <thread>
#include <vector>
#include <Core/Global.h>

class ThreadUtil
{
public:
	//
	// Sleeps for the given duration. Checks every 5ms to make sure node is being shutdown, and breaks if it is.
	//
	template<class _Rep, class _Period> inline
	static void SleepFor(const std::chrono::duration<_Rep, _Period>& duration)
	{
		std::chrono::time_point wakeTime = std::chrono::system_clock::now() + duration;
		while (Global::IsRunning()) {
			if (std::chrono::system_clock::now() >= wakeTime) {
				break;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
	}

	static void Join(std::thread& thread)
	{
		try
		{
			if (thread.joinable())
			{
				thread.join();
			}
		} catch (...) { }
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