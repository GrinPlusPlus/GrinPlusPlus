#pragma once

#include "../Wallet.h"

#include <Common/Secure.h>
#include <Crypto/SecretKey.h>
#include <Wallet/Slate.h>

class FinalizeSlateBuilder
{
public:
	std::unique_ptr<Slate> Finalize(Wallet& wallet, const SecureVector& masterSeed, const Slate& slate) const;

private:
	bool AddPartialSignature(Wallet& wallet, const SecureVector& masterSeed, Slate& slate, const Hash& kernelMessage) const;
	bool AddFinalTransaction(Slate& slate, const Hash& kernelMessage) const;

	bool UpdateDatabase(Wallet& wallet, const SecureVector& masterSeed, Slate& finalSlate) const;
};