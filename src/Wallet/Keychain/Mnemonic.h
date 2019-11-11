#pragma once

#include <Common/Secure.h>
#include <optional>
#include <vector>

class Mnemonic
{
public:
	static SecureString CreateMnemonic(const std::vector<unsigned char>& entropy);
	static SecureVector ToEntropy(const SecureString& walletWords);
};