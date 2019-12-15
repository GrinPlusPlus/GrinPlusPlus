#pragma once

#include "Wallet.h"

#include <Common/Secure.h>
#include <vector>
#include <mutex>

struct LoggedInSession
{
	LoggedInSession(Locked<Wallet> wallet, SecureVector&& encryptedSeedWithCS)
		: m_wallet(wallet), m_encryptedSeedWithCS(std::move(encryptedSeedWithCS))
	{

	}

	Locked<Wallet> m_wallet;
	SecureVector m_encryptedSeedWithCS;
};