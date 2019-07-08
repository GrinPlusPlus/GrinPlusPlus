#pragma once

#include "../Wallet.h"

#include <Config/Config.h>
#include <Crypto/SecretKey.h>
#include <Wallet/WalletTx.h>
#include <Wallet/NodeClient.h>
#include <Wallet/SelectionStrategy.h>
#include <Wallet/Slate.h>
#include <optional>

class SendSlateBuilder
{
public:
	SendSlateBuilder(const Config& config, const INodeClient& nodeClient);

	//
	// Creates a slate for sending grins from the provided wallet.
	//
	std::unique_ptr<Slate> BuildSendSlate(
		Wallet& wallet, 
		const SecureVector& masterSeed, 
		const uint64_t amount, 
		const uint64_t feeBase, 
		const uint8_t numOutputs, 
		const std::optional<std::string>& messageOpt, 
		const ESelectionStrategy& strategy) const;

private:
	SecretKey CalculatePrivateKey(const BlindingFactor& transactionOffset, const std::vector<OutputData>& inputs, const std::vector<OutputData>& changeOutputs) const;
	void AddSenderInfo(Slate& slate, const SecretKey& secretKey, const SecretKey& secretNonce, const std::optional<std::string>& messageOpt) const;
	WalletTx BuildWalletTx(Wallet& wallet, const uint32_t walletTxId, const std::vector<OutputData>& inputs, const std::vector<OutputData>& changeOutputs, const Slate& slate, const std::optional<std::string>& messageOpt) const;

	bool UpdateDatabase(Wallet& wallet, const SecureVector& masterSeed, const uuids::uuid& slateId, const SlateContext& context, const std::vector<OutputData>& changeOutputs, std::vector<OutputData>& coinsToLock, const WalletTx& walletTx) const;

	const Config& m_config;
	const INodeClient& m_nodeClient;
};