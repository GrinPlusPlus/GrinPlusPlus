#pragma once

#include "Wallet.h"

#include <uuid.h>
#include <Common/Secure.h>
#include <Wallet/Slate.h>
#include <Wallet/NodeClient.h>
#include <BlockChainServer.h>
#include <Core/Transaction.h>

class Sender
{
public:
	Sender(const INodeClient& nodeClient);

	// Creates a slate for sending grins from the provided wallet.
	std::unique_ptr<Slate> BuildSendSlate(Wallet& wallet, const SecureString& password, const uint64_t amount, const uint64_t feeBase, const std::string& message, const ESelectionStrategy& strategy) const; // TODO: Password?

private:
	std::vector<WalletCoin> SelectCoinsToSpend(Wallet& wallet, const SecureString& password, const uint64_t amount, const uint64_t feeBase, const ESelectionStrategy& strategy, const int64_t numOutputs, const int64_t numKernels) const;
	std::unique_ptr<WalletCoin> CreateChangeOutput(Wallet& wallet, const SecureString& password, const std::vector<WalletCoin>& inputs, const uint64_t amount, const uint64_t fee) const;
	Transaction BuildTransaction(const std::vector<WalletCoin>& inputs, const WalletCoin& changeOutput, const BlindingFactor& transactionOffset, const uint64_t fee, const uint64_t lockHeight) const;

	const INodeClient& m_nodeClient;
};