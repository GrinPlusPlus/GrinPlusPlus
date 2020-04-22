#pragma once

#include "../Wallet.h"

#include <Common/Secure.h>
#include <Crypto/SecretKey.h>
#include <Wallet/Models/Slate/Slate.h>

class FinalizeSlateBuilder
{
public:
	Slate Finalize(
		Locked<Wallet> wallet,
		const SecureVector& masterSeed,
		const Slate& slate
	) const;

private:
	bool AddPartialSignature(
		const Wallet::CPtr& pWallet,
		const SecureVector& masterSeed,
		Slate& slate,
		const Hash& kernelMessage
	) const;

	bool AddFinalTransaction(
		Slate& slate,
		const Hash& kernelMessage,
		const Commitment& finalExcess
	) const;

	bool VerifyPaymentProofs(
		const std::unique_ptr<WalletTx>& pWalletTx,
		const Slate& slate,
		const Commitment& finalExcess
	) const;

	void UpdateDatabase(
		const Wallet::Ptr& pWallet,
		const SecureVector& masterSeed,
		Slate& finalSlate
	) const;
};