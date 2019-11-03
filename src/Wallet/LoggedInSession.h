#pragma once

#include "Wallet.h"

#include <Common/Secure.h>
#include <vector>
#include <mutex>

struct LoggedInSession
{
	LoggedInSession(Locked<Wallet> wallet, SecureVector&& encryptedSeedWithCS, SecureVector&& encryptedGrinboxAddress)
		: m_wallet(wallet), m_encryptedSeedWithCS(std::move(encryptedSeedWithCS)), m_encryptedGrinboxAddress(std::move(encryptedGrinboxAddress))
	{

	}

	Locked<Wallet> m_wallet;
	SecureVector m_encryptedSeedWithCS;
	SecureVector m_encryptedGrinboxAddress;
};