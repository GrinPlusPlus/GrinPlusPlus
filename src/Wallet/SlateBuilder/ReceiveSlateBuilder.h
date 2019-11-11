#pragma once

#include "../Wallet.h"

#include <Common/Secure.h>
#include <Wallet/Slate.h>
#include <optional>

class ReceiveSlateBuilder
{
public:
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
	void UpdateDatabase(
		Writer<IWalletDB> pBatch,
		const SecureVector& masterSeed,
		const Slate& slate,
		const OutputData& outputData,
		const uint32_t walletTxId,
		const SlateContext& context,
		const std::optional<std::string>& addressOpt,
		const std::optional<std::string>& messageOpt
	) const;
};