#pragma once

#include "../Wallet.h"

#include <Common/Secure.h>
#include <Crypto/SecretKey.h>
#include <Wallet/Models/Slate/Slate.h>

class FinalizeSlateBuilder
{
public:
	std::pair<Slate, Transaction> Finalize(
		Locked<Wallet> wallet,
		const SecureVector& masterSeed,
		const Slate& rcvSlate
	) const;

private:
	bool AddPartialSignature(
		const SlateContextEntity& context,
		Slate& finalizeSlate,
		const Hash& kernelMessage
	) const;

	std::unique_ptr<Transaction> BuildTransaction(
		Slate& finalizeSlate,
		const Hash& kernelMessage,
		const Commitment& finalExcess
	) const;

	bool VerifyPaymentProofs(
		const std::unique_ptr<WalletTx>& pWalletTx,
		const Slate& finalizeSlate,
		const Commitment& finalExcess
	) const;

	void UpdateDatabase(
		const Wallet::Ptr& pWallet,
		const SecureVector& masterSeed,
		const WalletTx& walletTx,
		const Slate& finalizeSlate
	) const;
};