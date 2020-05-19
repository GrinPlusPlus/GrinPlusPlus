#pragma once

#include <Core/Models/DTOs/OutputDTO.h>

class OutputRange
{
public:
	OutputRange(const uint64_t highestIndex, const uint64_t lastRetrievedIndex, std::vector<OutputDTO>&& outputs)
		: m_highestIndex(highestIndex), m_lastRetrievedIndex(lastRetrievedIndex), m_outputs(std::move(outputs))
	{

	}

	uint64_t GetHighestIndex() const { return m_highestIndex; }
	uint64_t GetLastRetrievedIndex() const { return m_lastRetrievedIndex; }
	const std::vector<OutputDTO>& GetOutputs() const { return m_outputs; }

	static OutputRange FromJSON(const Json::Value& json)
	{
		const uint64_t highestIndex = JsonUtil::GetRequiredUInt64(json, "highest_index");
		const uint64_t lastRetrievedIndex = JsonUtil::GetRequiredUInt64(json, "last_retrieved_index");
		Json::Value outputsJson = JsonUtil::GetRequiredField(json, "outputs");
		if (!outputsJson.isArray())
		{
			throw DESERIALIZATION_EXCEPTION("outputs must be an array");
		}

		std::vector<OutputDTO> outputs;
		outputs.reserve(outputsJson.size());
		for (const Json::Value& output : outputsJson)
		{
			outputs.push_back(OutputDTO::FromJSON(output));
		}

		return OutputRange(highestIndex, lastRetrievedIndex, std::move(outputs));
	}

private:
	uint64_t m_highestIndex;
	uint64_t m_lastRetrievedIndex;
	std::vector<OutputDTO> m_outputs;
};