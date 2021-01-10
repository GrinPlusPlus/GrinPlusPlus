#pragma once

#include "../WalletImpl.h"
#include "SigningKeys.h"

#include <Common/Secure.h>
#include <Wallet/Models/Slate/Slate.h>
#include <optional>

class ReceiveSlateBuilder
{
public:
	ReceiveSlateBuilder(const Config& config)
		: m_config(config) { }

	Slate AddReceiverData(
		Locked<WalletImpl> wallet,
		const SecureVector& masterSeed,
		const Slate& slate,
		const std::optional<SlatepackAddress>& addressOpt,
		const std::vector<SlatepackAddress>& recipients
	) const;

private:
	bool VerifySlateStatus(std::shared_ptr<WalletImpl> pWallet, const SecureVector& masterSeed, const Slate& slate) const;
	SlateSignature BuildSignature(Slate& slate, const SigningKeys& signing_keys) const;
	void UpdatePaymentProof(
		const std::shared_ptr<WalletImpl>& pWallet,
		const SecureVector& masterSeed,
		Slate& receiveSlate
	) const;
	void UpdateDatabase(
		std::shared_ptr<IWalletDB> pBatch,
		const SecureVector& masterSeed,
		const Slate& receiveSlate,
		const OutputDataEntity& outputData,
		const uint32_t walletTxId,
		const std::optional<SlatepackAddress>& addressOpt,
		const std::string& armored_slatepack
	) const;

	const Config& m_config;
};