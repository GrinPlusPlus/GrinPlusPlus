#pragma once

#include <Common/Secure.h>
#include <optional>
#include <vector>

class Mnemonic
{
public:
	static SecureString CreateMnemonic(const std::vector<unsigned char>& entropy, const std::optional<SecureString>& password);
};