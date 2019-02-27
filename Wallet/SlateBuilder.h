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
	std::unique_ptr<Slate> BuildSendSlate(Wallet& wallet, const CBigInteger<32>& masterSeed, const uint64_t amount, const uint64_t feeBase, const std::string& message, const ESelectionStrategy& strategy) const;
	bool AddReceiverData(Wallet& wallet, const CBigInteger<32>& masterSeed, Slate& slate) const;
	std::unique_ptr<Transaction> Finalize(Wallet& wallet, const CBigInteger<32>& masterSeed, Slate& slate) const;

private:
	std::vector<WalletCoin> SelectCoinsToSpend(Wallet& wallet, const CBigInteger<32>& masterSeed, const uint64_t amount, const uint64_t feeBase, const ESelectionStrategy& strategy, const int64_t numOutputs, const int64_t numKernels) const;
	std::unique_ptr<WalletCoin> CreateChangeOutput(Wallet& wallet, const CBigInteger<32>& masterSeed, const std::vector<WalletCoin>& inputs, const uint64_t amount, const uint64_t fee) const;
	Transaction BuildTransaction(const std::vector<WalletCoin>& inputs, const WalletCoin& changeOutput, const BlindingFactor& transactionOffset, const uint64_t fee, const uint64_t lockHeight) const;
	std::unique_ptr<Signature> GeneratePartialSignature(const BlindingFactor& secretKey, const BlindingFactor& secretNonce, const std::vector<ParticipantData>& participants, const Hash& message) const;

	const INodeClient& m_nodeClient;
};