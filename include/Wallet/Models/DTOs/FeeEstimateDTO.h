#pragma once

#include <Wallet/Models/DTOs/WalletOutputDTO.h>
#include <Core/Util/JsonUtil.h>

class FeeEstimateDTO
{
public:
	FeeEstimateDTO(const uint64_t amount, const uint64_t fee, std::vector<WalletOutputDTO>&& inputs)
		: m_amount(amount), m_fee(fee), m_inputs(std::move(inputs)) { }

	uint64_t GetAmount() const noexcept { return m_amount; }
	uint64_t GetFee() const noexcept { return m_fee; }
	const std::vector<WalletOutputDTO>& GetInputs() const noexcept { return m_inputs; }

	static FeeEstimateDTO FromJSON(const Json::Value& json)
	{
		const uint64_t amount = JsonUtil::GetRequiredUInt64(json, "amount");
		const uint64_t fee = JsonUtil::GetRequiredUInt64(json, "fee");

		std::vector<WalletOutputDTO> inputs;
		const auto inputsOpt = JsonUtil::GetOptionalField(json, "inputs");
		if (inputsOpt.has_value())
		{
			inputs.reserve(inputsOpt.value().size());
			for (auto iter = inputsOpt.value().begin(); iter != inputsOpt.value().end(); iter++)
			{
				inputs.push_back(WalletOutputDTO::FromJSON(*iter));
			}
		}

		return FeeEstimateDTO(amount, fee, std::move(inputs));
	}

	Json::Value ToJSON() const
	{
		Json::Value feeEstimateJSON;
		feeEstimateJSON["amount"] = m_amount;
		feeEstimateJSON["fee"] = m_fee;

		Json::Value inputsJSON;
		for (const WalletOutputDTO& input : m_inputs)
		{
			inputsJSON.append(input.ToJSON());
		}
		feeEstimateJSON["inputs"] = inputsJSON;
		
		return feeEstimateJSON;
	}

private:
	uint64_t m_amount;
	uint64_t m_fee;
	std::vector<WalletOutputDTO> m_inputs;
};