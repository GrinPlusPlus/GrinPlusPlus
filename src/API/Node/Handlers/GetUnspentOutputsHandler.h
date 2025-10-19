#pragma once

#include <BlockChain/BlockChain.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <optional>

class GetUnspentOutputsHandler : public RPCMethod
{
public:
	GetUnspentOutputsHandler(const ITxHashSetPtr& pTxHashSet, const IDatabasePtr& pDatabase)
		: m_pTxHashSet(pTxHashSet), m_pDatabase(pDatabase) { }
	~GetUnspentOutputsHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		uint64_t startIndex = 1;
		uint64_t max = 100;

		const Json::Value params_json = request.GetParams().value();

		auto startIndexParam = JsonUtil::GetUInt64Opt(params_json, "start_index");
		auto maxParam = JsonUtil::GetUInt64Opt(params_json, "max");
		if (startIndexParam.has_value())
		{
			startIndex = startIndexParam.value();
		}
		if (maxParam.has_value())
		{
			max = maxParam.value();
		}

		auto pBlockDB = m_pDatabase->GetBlockDB()->Read();
		OutputRange range = m_pTxHashSet->GetOutputsByLeafIndex(pBlockDB.GetShared(), startIndex, max);

		Json::Value outputs;
		for (const OutputDTO& info : range.GetOutputs())
		{
			if (info.IsSpent())
			{
				continue;
			}
			Json::Value output;
			output["output_type"] = OutputFeatures::ToString(info.GetIdentifier().GetFeatures());
			output["commit"] = info.GetIdentifier().GetCommitment().ToHex();
			output["spent"] = info.IsSpent();
			output["proof"] = info.GetRangeProof().Format();

			Serializer proofSerializer;
			info.GetRangeProof().Serialize(proofSerializer);
			output["proof_hash"] = Hasher::Blake2b(proofSerializer.GetBytes()).ToHex();

			output["block_height"] = info.GetLocation().GetBlockHeight();
			output["merkle_proof"] = Json::nullValue;
			output["mmr_index"] = info.GetLeafIndex().GetPosition() + 1;

			outputs.append(output);
		}

		Json::Value result;
		result["Ok"] = outputs;

		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	ITxHashSetPtr m_pTxHashSet;
	IDatabasePtr m_pDatabase;
};