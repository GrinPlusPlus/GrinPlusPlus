#pragma once

#include <Wallet/Models/DTOs/WalletOutputDTO.h>
#include <Core/Util/JsonUtil.h>

class FeeEstimateDTO
{
public:
	FeeEstimateDTO(const uint64_t fee, std::vector<WalletOutputDTO>&& inputs)
		: m_fee(fee), m_inputs(std::move(inputs))
	{

	}

	Json::Value ToJSON() const
	{
		Json::Value feeEstimateJSON;
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
	uint64_t m_fee;
	std::vector<WalletOutputDTO> m_inputs;
};