#pragma once

#include "Wallet.h"

#include <vector>
#include <mutex>

struct LoggedInSession
{
	LoggedInSession(Locked<Wallet> wallet, std::vector<unsigned char>&& encryptedSeedWithCS, std::vector<unsigned char>&& encryptedGrinboxAddress)
		: m_wallet(wallet), m_encryptedSeedWithCS(std::move(encryptedSeedWithCS)), m_encryptedGrinboxAddress(std::move(encryptedGrinboxAddress))
	{

	}

	Locked<Wallet> m_wallet;
	std::vector<unsigned char> m_encryptedSeedWithCS;
	std::vector<unsigned char> m_encryptedGrinboxAddress;
};