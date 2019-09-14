#pragma once

#include <Common/Secure.h>
#include <optional>
#include <vector>

class Mnemonic
{
public:
	// TODO: Use SecureVectors
	static SecureString CreateMnemonic(const std::vector<unsigned char>& entropy);
	static std::optional<SecureVector> ToEntropy(const SecureString& walletWords);
};