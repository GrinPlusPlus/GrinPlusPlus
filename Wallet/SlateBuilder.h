#pragma once

#include "Wallet.h"

#include <uuid.h>
#include <Common/Secure.h>
#include <Wallet/Slate.h>
#include <Wallet/NodeClient.h>
#include <BlockChain/BlockChainServer.h>
#include <Core/Models/Transaction.h>

class SlateBuilder
{
public:
	SlateBuilder(const INodeClient& nodeClient);

	// Creates a slate for sending grins from the provided wallet.
	std::unique_ptr<Slate> BuildSendSlate(Wallet& wallet, const CBigInteger<32>& masterSeed, const uint64_t amount, const uint64_t feeBase, const std::optional<std::string>& messageOpt, const ESelectionStrategy& strategy) const;
	bool AddReceiverData(Wallet& wallet, const CBigInteger<32>& masterSeed, Slate& slate, const std::optional<std::string>& messageOpt) const;
	std::unique_ptr<Transaction> Finalize(Wallet& wallet, const CBigInteger<32>& masterSeed, const Slate& slate) const;

private:
	std::unique_ptr<OutputData> CreateChangeOutput(Wallet& wallet, const CBigInteger<32>& masterSeed, const std::vector<OutputData>& inputs, const uint64_t amount, const uint64_t fee) const;
	bool SaveWalletTx(const CBigInteger<32>& masterSeed, Wallet& wallet, const Slate& slate, const EWalletTxType type) const;

	const INodeClient& m_nodeClient;
};