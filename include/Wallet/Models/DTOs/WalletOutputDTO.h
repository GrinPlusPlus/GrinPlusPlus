#pragma once

#include <Wallet/OutputData.h>
#include <Core/Util/JsonUtil.h>

class WalletOutputDTO
{
public:
	WalletOutputDTO(const OutputData& outputData)
		: m_outputData(outputData)
	{

	}

	Json::Value ToJSON() const
	{
		Json::Value outputJSON;

		outputJSON["keychain_path"] = m_outputData.GetKeyChainPath().ToString();
		outputJSON["commitment"] = m_outputData.GetOutput().GetCommitment().ToHex();
		outputJSON["amount"] = m_outputData.GetAmount();
		outputJSON["status"] = OutputStatus::ToString(m_outputData.GetStatus());

		JsonUtil::AddOptionalField(outputJSON, "mmr_index", m_outputData.GetMMRIndex());
		JsonUtil::AddOptionalField(outputJSON, "block_height", m_outputData.GetBlockHeight());
		JsonUtil::AddOptionalField(outputJSON, "transaction_id", m_outputData.GetWalletTxId());

		return outputJSON;
	}

private:
	OutputData m_outputData;
};