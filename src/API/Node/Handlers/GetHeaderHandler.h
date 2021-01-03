#pragma once

#include <BlockChain/BlockChain.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <optional>

class GetHeaderHandler : public RPCMethod
{
public:
	GetHeaderHandler(const IBlockChain::Ptr& pBlockChain)
		: m_pBlockChain(pBlockChain) { }
	~GetHeaderHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value()) {
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		const Json::Value params = request.GetParams().value();
		if (!params.isArray() || params.size() < 3) {
			return request.BuildError("INVALID_PARAMS", "Expected 3 parameters: height, hash, commit");
		}

		BlockHeaderPtr pHeader = nullptr;
		if (!params[0].isNull()) {
			const uint64_t height = JsonUtil::ConvertToUInt64(params[0]);
			pHeader = m_pBlockChain->GetBlockHeaderByHeight(height, EChainType::CONFIRMED);
		}

		if (!params[1].isNull()) {
			const Hash hash = JsonUtil::ConvertToHash(params[1]);
			pHeader = m_pBlockChain->GetBlockHeaderByHash(hash);
		}

		if (!params[2].isNull()) {
			const Commitment commitment = JsonUtil::ConvertToCommitment(params[2]);
			pHeader = m_pBlockChain->GetBlockHeaderByCommitment(commitment);
		}

		if (pHeader == nullptr) {
			return request.BuildError("NOT_FOUND", "Header not found");
		}

		Json::Value result;
		result["Ok"] = pHeader->ToJSON();
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	IBlockChain::Ptr m_pBlockChain;
};