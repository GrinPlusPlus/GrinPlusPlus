#pragma once

#include <Wallet/PrivateExtKey.h>
#include <Wallet/PublicExtKey.h>

#include <memory>

class PublicKeyCalculator
{
public:
	std::unique_ptr<PublicExtKey> DeterminePublicKey(const PrivateExtKey& privateKey) const;
};