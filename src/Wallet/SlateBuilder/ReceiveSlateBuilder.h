#pragma once

#include "../Wallet.h"

#include <Common/Secure.h>
#include <Wallet/Models/Slate/Slate.h>
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
		const std::optional<std::string>& addressOpt
	) const;

private:
	bool VerifySlateStatus(std::shared_ptr<Wallet> pWallet, const SecureVector& masterSeed, const Slate& slate) const;
	SlateSignature BuildSignature(Slate& slate, const SecretKey& secretKey, const SecretKey& secretNonce) const;
	void UpdatePaymentProof(
		std::shared_ptr<Wallet> pWallet,
		IWalletDBPtr pWalletDB,
		const SecureVector& masterSeed,
		Slate& receiveSlate
	) const;
	void UpdateDatabase(
		std::shared_ptr<IWalletDB> pBatch,
		const SecureVector& masterSeed,
		Slate& receiveSlate,
		const SlateSignature& signature,
		const OutputDataEntity& outputData,
		const uint32_t walletTxId,
		const std::optional<std::string>& addressOpt
	) const;

	const Config& m_config;
};