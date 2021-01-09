#pragma once

#include <Core/Util/JsonUtil.h>
#include <Wallet/SessionToken.h>
#include <Wallet/Models/DTOs/PostMethodDTO.h>
#include <Wallet/Models/DTOs/SelectionStrategyDTO.h>

class EstimateFeeCriteria
{
public:
	EstimateFeeCriteria(
		const SessionToken& token,
		const std::optional<uint64_t>& amount,
		const uint64_t feeBase,
		const uint8_t changeOutputs,
		const SelectionStrategyDTO& selectionStrategy
	)	: m_token(token),
		m_amount(amount),
		m_feeBase(feeBase),
		m_changeOutputs(changeOutputs),
		m_selectionStrategy(selectionStrategy) { }

	static EstimateFeeCriteria FromJSON(const Json::Value& json)
	{
		const SessionToken token = SessionToken::FromBase64(JsonUtil::GetRequiredString(json, "session_token"));
		std::optional<uint64_t> amountOpt = JsonUtil::GetUInt64Opt(json, "amount");
		const uint64_t feeBase = JsonUtil::GetRequiredUInt64(json, "fee_base");
		const Json::Value selectionStrategyJSON = JsonUtil::GetRequiredField(json, "selection_strategy");
		const SelectionStrategyDTO selectionStrategy = SelectionStrategyDTO::FromJSON(selectionStrategyJSON);
		uint8_t changeOutputs = (uint8_t)json.get("change_outputs", Json::Value(1)).asUInt();

		// No change outputs when sending all
		if (!amountOpt.has_value()) {
			changeOutputs = 0;
		}

		return EstimateFeeCriteria(
			token,
			amountOpt,
			feeBase,
			changeOutputs,
			selectionStrategy
		);
	}

	//
	// Getters
	//
	const SessionToken& GetToken() const noexcept { return m_token; }
	uint64_t GetAmount() const noexcept { return m_amount.value_or(0); }
	uint64_t GetFeeBase() const noexcept { return m_feeBase; }
	uint8_t GetNumChangeOutputs() const noexcept { return m_changeOutputs; }
	const SelectionStrategyDTO& GetSelectionStrategy() const noexcept { return m_selectionStrategy; }

	bool SendEntireBalance() const noexcept { return !m_amount.has_value(); }

private:
	SessionToken m_token;
	std::optional<uint64_t> m_amount;
	uint64_t m_feeBase;
	uint8_t m_changeOutputs;
	SelectionStrategyDTO m_selectionStrategy;
};