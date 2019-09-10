#pragma once

#include <Common/Util/TimeUtil.h>
#include <queue>

class RateCounter
{
public:
	void AddMessageReceived()
	{
		m_received.push(TimeUtil::Now());
	}

	void AddMessageSent()
	{
		m_sent.push(TimeUtil::Now());
	}

	size_t GetReceivedInLastMinute()
	{

	}

	size_t GetSentInLastMinute()
	{

	}

private:
	std::queue<std::time_t> m_sent;
	std::queue<std::time_t> m_received;
};