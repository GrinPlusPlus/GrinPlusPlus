#pragma once

#include <Wallet/Models/Slatepack/SlatepackMessage.h>

struct ISlatepackDecryptor
{
	virtual SlatepackMessage Decrypt(const SessionToken& token, const std::string& armored) const = 0;
};