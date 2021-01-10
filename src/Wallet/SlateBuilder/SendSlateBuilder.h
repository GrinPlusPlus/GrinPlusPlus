#pragma once

#include "../WalletImpl.h"

#include <Core/Config.h>
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
		Locked<WalletImpl> wallet, 
		const SecureVector& masterSeed, 
		const uint64_t amount, 
		const uint64_t feeBase, 
		const uint8_t changeOutputs,
		const bool sendEntireBalance,
		const std::optional<std::string>& addressOpt,
		const SelectionStrategyDTO& strategy,
		const uint16_t slateVersion,
		const std::vector<SlatepackAddress>& recipients
	) const;

private:
	Slate Build(
		const std::shared_ptr<WalletImpl>& pWallet,
		const SecureVector& masterSeed,
		const uint64_t amountToSend,
		const uint64_t fee,
		const uint8_t numChangeOutputs,
		std::vector<OutputDataEntity>& inputs,
		const std::optional<std::string>& addressOpt,
		const uint16_t slateVersion,
		const std::vector<SlatepackAddress>& recipients
	) const;

	WalletTx BuildWalletTx(
		const uint32_t walletTxId,
		const BlindingFactor& txOffset,
		const std::vector<OutputDataEntity>& inputs,
		const std::vector<OutputDataEntity>& changeOutputs,
		const Slate& slate,
		const std::optional<std::string>& addressOpt,
		const std::optional<SlatePaymentProof>& proofOpt
	) const;

	void UpdateDatabase(
		std::shared_ptr<IWalletDB> pBatch,
		const SecureVector& masterSeed,
		const Slate& slate,
		const SlateContextEntity& context,
		const std::vector<OutputDataEntity>& changeOutputs,
		std::vector<OutputDataEntity>& coinsToLock,
		const WalletTx& walletTx,
		const std::string& armored_slatepack
	) const;

	const Config& m_config;
	INodeClientConstPtr m_pNodeClient;
};