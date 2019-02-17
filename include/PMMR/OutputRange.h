#pragma once

#include <Core/Models/Display/OutputDisplayInfo.h>

class OutputRange
{
public:
	OutputRange(const uint64_t highestIndex, const uint64_t lastRetrievedIndex, std::vector<OutputDisplayInfo>&& outputs)
		: m_highestIndex(highestIndex), m_lastRetrievedIndex(lastRetrievedIndex), m_outputs(std::move(outputs))
	{

	}

	inline uint64_t GetHighestIndex() const { return m_highestIndex; }
	inline uint64_t GetLastRetrievedIndex() const { return m_lastRetrievedIndex; }
	inline const std::vector<OutputDisplayInfo>& GetOutputs() const { return m_outputs; }

private:
	uint64_t m_highestIndex;
	uint64_t m_lastRetrievedIndex;
	std::vector<OutputDisplayInfo> m_outputs;
};