#pragma once

#include <Wallet/Enums/SelectionStrategy.h>
#include <Crypto/Commitment.h>
#include <Core/Util/JsonUtil.h>
#include <set>

class SelectionStrategyDTO
{
public:
	SelectionStrategyDTO(const ESelectionStrategy& strategy, std::set<Commitment>&& inputs)
		: m_strategy(strategy), m_inputs(std::move(inputs))
	{

	}

	static SelectionStrategyDTO FromJSON(const Json::Value& selectionStrategyJSON)
	{
		const std::optional<std::string> selectionStrategyOpt = JsonUtil::GetStringOpt(selectionStrategyJSON, "strategy");
		if (!selectionStrategyOpt.has_value())
		{
			throw DeserializationException();
		}

		std::set<Commitment> inputs;
		const Json::Value inputsJSON = JsonUtil::GetOptionalField(selectionStrategyJSON, "inputs");
		if (inputsJSON != Json::nullValue)
		{
			for (Json::Value::const_iterator iter = inputsJSON.begin(); iter != inputsJSON.end(); iter++)
			{
				inputs.insert(JsonUtil::ConvertToCommitment(*iter, true));
			}
		}

		return SelectionStrategyDTO(SelectionStrategy::FromString(selectionStrategyOpt.value()), std::move(inputs));
	}

	inline const ESelectionStrategy GetStrategy() const { return m_strategy; }
	inline const std::set<Commitment>& GetInputs() const { return m_inputs; }

private:
	ESelectionStrategy m_strategy;
	std::set<Commitment> m_inputs;
};