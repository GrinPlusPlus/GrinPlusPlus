#pragma once

#include <Core/Validation/TransactionValidator.h>
#include <BlockChain/BlockChain.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <optional>

class PushTransactionHandler : public RPCMethod
{
public:
	PushTransactionHandler(const IBlockChain::Ptr& pBlockChain, const IP2PServerPtr& pP2PServer)
		: m_pBlockChain(pBlockChain), m_pP2PServer(pP2PServer) { }
	~PushTransactionHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value()) {
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		const Json::Value params = request.GetParams().value();
		if (!params.isArray() || params.size() < 1 || params[0].isNull()) {
			return request.BuildError("INVALID_PARAMS", "Expected 2 parameters: transaction, fluff");
		}

		TransactionPtr pTransaction = std::make_shared<Transaction>(Transaction::FromJSON(params[0]));

		bool fluff = false;
		if (params.size() >=2 && !params[1].isNull()) {
			fluff = params[1].asBool();
		}

		const uint64_t block_height = m_pBlockChain->GetHeight(EChainType::CONFIRMED);
		TransactionValidator().Validate(*pTransaction, block_height);

		EBlockChainStatus status = m_pBlockChain->AddTransaction(
			pTransaction,
			fluff ? EPoolType::MEMPOOL : EPoolType::STEMPOOL
		);

		if (status == EBlockChainStatus::SUCCESS) {
			m_pP2PServer->BroadcastTransaction(pTransaction);

			Json::Value result;
			result["Ok"] = Json::Value(Json::nullValue);
			return request.BuildResult(result);
		} else {
			return request.BuildError("TX_POOL_ERROR", "Failed to submit transaction to the transaction pool");
		}
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	IBlockChain::Ptr m_pBlockChain;
	IP2PServerPtr m_pP2PServer;
};