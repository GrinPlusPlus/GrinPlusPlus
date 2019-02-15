#pragma once

#include <PMMR/OutputInfo.h>

class OutputRange
{
public:
	OutputRange(const uint64_t highestIndex, const uint64_t lastRetrievedIndex, std::vector<OutputInfo>&& outputs)
		: m_highestIndex(highestIndex), m_lastRetrievedIndex(lastRetrievedIndex), m_outputs(std::move(outputs))
	{

	}

	inline uint64_t GetHighestIndex() const { return m_highestIndex; }
	inline uint64_t GetLastRetrievedIndex() const { return m_lastRetrievedIndex; }
	inline const std::vector<OutputInfo>& GetOutputs() const { return m_outputs; }

private:
	uint64_t m_highestIndex;
	uint64_t m_lastRetrievedIndex;
	std::vector<OutputInfo> m_outputs;
};