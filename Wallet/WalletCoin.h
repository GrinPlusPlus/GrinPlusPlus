#pragma once

#include "Keychain/Keys/PrivateExtKey.h"

#include <Core/TransactionOutput.h>

class WalletCoin
{
public:
	inline const PrivateExtKey& GetPrivateKey() const { return m_privateKey; }
	inline const TransactionOutput& GetOutput() const { return m_output; }
	inline const uint64_t GetAmount() const { return m_amount; }

private:
	PrivateExtKey m_privateKey;
	TransactionOutput m_output;
	uint64_t m_amount;
	// TODO: EOutputStatus
};