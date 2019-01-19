#pragma once

#include "Keys/PrivateExtKey.h"
#include "Keys/PublicExtKey.h"

#include <memory>

class PublicKeyCalculator
{
public:
	std::unique_ptr<PublicExtKey> DeterminePublicKey(const PrivateExtKey& privateKey) const;
};