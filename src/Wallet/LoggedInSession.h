#pragma once

#include "WalletImpl.h"
#include <Wallet/Wallet.h>

#include <Common/Secure.h>

struct LoggedInSession
{
	LoggedInSession(const Locked<Wallet>& wallet, const Locked<WalletImpl>& walletImpl, SecureVector&& encryptedSeedWithCS)
		: m_wallet(wallet), m_walletImpl(walletImpl), m_encryptedSeedWithCS(std::move(encryptedSeedWithCS)) { }

	Locked<Wallet> m_wallet;
	Locked<WalletImpl> m_walletImpl;
	SecureVector m_encryptedSeedWithCS;
};