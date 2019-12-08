#pragma once

#include "../Wallet.h"

#include <Common/Secure.h>
#include <Crypto/SecretKey.h>
#include <Wallet/Slate.h>

class FinalizeSlateBuilder
{
public:
	Slate Finalize(Locked<Wallet> wallet, const SecureVector& masterSeed, const Slate& slate) const;

private:
	bool AddPartialSignature(std::shared_ptr<const Wallet> pWallet, const SecureVector& masterSeed, Slate& slate, const Hash& kernelMessage) const;
	bool AddFinalTransaction(Slate& slate, const Hash& kernelMessage, const Commitment& finalExcess) const;
	bool VerifyPaymentProofs(const std::unique_ptr<WalletTx>& pWalletTx, const Slate& slate, const Commitment& finalExcess) const;

	void UpdateDatabase(std::shared_ptr<Wallet> pWallet, const SecureVector& masterSeed, Slate& finalSlate) const;
};