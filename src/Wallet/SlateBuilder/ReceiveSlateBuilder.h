#pragma once

#include "../Wallet.h"

#include <Common/Secure.h>
#include <Wallet/Slate.h>
#include <optional>

class ReceiveSlateBuilder
{
public:
	ReceiveSlateBuilder(const Config& config)
		: m_config(config)
	{

	}

	Slate AddReceiverData(
		Locked<Wallet> wallet,
		const SecureVector& masterSeed,
		const Slate& slate,
		const std::optional<std::string>& addressOpt,
		const std::optional<std::string>& messageOpt
	) const;

private:
	bool VerifySlateStatus(std::shared_ptr<Wallet> pWallet, const SecureVector& masterSeed, const Slate& slate) const;
	void AddParticipantData(Slate& slate, const SecretKey& secretKey, const SecretKey& secretNonce, const std::optional<std::string>& messageOpt) const;
	void UpdatePaymentProof(std::shared_ptr<Wallet> pWallet, IWalletDBPtr pWalletDB, const SecureVector& masterSeed, Slate& slate) const;
	void UpdateDatabase(
		Writer<IWalletDB> pBatch,
		const SecureVector& masterSeed,
		const Slate& slate,
		const OutputData& outputData,
		const uint32_t walletTxId,
		const std::optional<std::string>& addressOpt,
		const std::optional<std::string>& messageOpt
	) const;

	const Config& m_config;
};