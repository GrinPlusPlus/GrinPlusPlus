#pragma once

#include <Common/Secure.h>
#include <optional>
#include <vector>

class Mnemonic
{
public:
	static SecureString CreateMnemonic(const std::vector<unsigned char>& entropy, const std::optional<SecureString>& password);
	static std::optional<std::vector<unsigned char>> ToEntropy(const SecureString& walletWords, const std::optional<SecureString>& password);
};