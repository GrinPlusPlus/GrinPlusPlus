#pragma once

#include "../Wallet.h"

#include <Config/Config.h>
#include <Crypto/SecretKey.h>
#include <Wallet/WalletTx.h>
#include <Wallet/NodeClient.h>
#include <Wallet/Models/DTOs/SelectionStrategyDTO.h>
#include <Wallet/Models/Slate/Slate.h>
#include <Wallet/WalletDB/Models/SlateContextEntity.h>
#include <optional>

class SendSlateBuilder
{
public:
	SendSlateBuilder(const Config& config, INodeClientConstPtr pNodeClient);

	//
	// Creates a slate for sending grins from the provided wallet.
	//
	Slate BuildSendSlate(
		Locked<Wallet> wallet, 
		const SecureVector& masterSeed, 
		const uint64_t amount, 
		const uint64_t feeBase, 
		const uint8_t numOutputs,
		const bool noChange,
		const std::optional<std::string>& addressOpt,
		const std::optional<std::string>& messageOpt, 
		const SelectionStrategyDTO& strategy,
		const uint16_t slateVersion) const;

private:
	SecretKey CalculatePrivateKey(const BlindingFactor& transactionOffset, const std::vector<OutputDataEntity>& inputs, const std::vector<OutputDataEntity>& changeOutputs) const;
	void AddSenderInfo(Slate& slate, const SecretKey& secretKey, const SecretKey& secretNonce, const std::optional<std::string>& messageOpt) const;
	WalletTx BuildWalletTx(
		const uint32_t walletTxId,
		const std::vector<OutputDataEntity>& inputs,
		const std::vector<OutputDataEntity>& changeOutputs,
		const Slate& slate,
		const std::optional<std::string>& addressOpt,
		const std::optional<std::string>& messageOpt,
		const std::optional<SlatePaymentProof>& proofOpt
	) const;

	void UpdateDatabase(
		Writer<IWalletDB> pBatch,
		const SecureVector& masterSeed,
		const uuids::uuid& slateId,
		const SlateContextEntity& context,
		const std::vector<OutputDataEntity>& changeOutputs,
		std::vector<OutputDataEntity>& coinsToLock,
		const WalletTx& walletTx
	) const;

	const Config& m_config;
	INodeClientConstPtr m_pNodeClient;
};