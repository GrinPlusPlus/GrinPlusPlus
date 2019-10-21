#pragma once

#include <Common/Util/TimeUtil.h>
#include <mutex>
#include <queue>

class RateCounter
{
public:
	RateCounter() = default;
	~RateCounter() = default;

	RateCounter(const RateCounter& other)
	{
		m_received = other.m_received;
		m_sent = other.m_sent;
	}

	void AddMessageReceived()
	{
		std::unique_lock<std::mutex> lock(m_receivedMutex);
		m_received.push(TimeUtil::Now());
	}

	void AddMessageSent()
	{
		std::unique_lock<std::mutex> lock(m_sentMutex);
		m_sent.push(TimeUtil::Now());
	}

	size_t GetReceivedInLastMinute()
	{
		std::unique_lock<std::mutex> lock(m_receivedMutex);
		std::time_t minuteAgo = time(nullptr) - 60;
		while (!m_received.empty())
		{
			if (minuteAgo <= m_received.front())
			{
				break;
			}

			m_received.pop();
		}

		return m_received.size();
	}

	size_t GetSentInLastMinute()
	{
		std::unique_lock<std::mutex> lock(m_sentMutex);
		std::time_t minuteAgo = time(nullptr) - 60;
		while (!m_sent.empty())
		{
			if (minuteAgo <= m_sent.front())
			{
				break;
			}

			m_sent.pop();
		}

		return m_sent.size();
	}

private:
	std::mutex m_receivedMutex;
	std::queue<std::time_t> m_received;

	std::mutex m_sentMutex;
	std::queue<std::time_t> m_sent;
};