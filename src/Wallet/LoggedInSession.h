#pragma once

#include "Wallet.h"

#include <mutex>
#include <vector>

struct LoggedInSession
{
    LoggedInSession(Wallet *pWallet, std::vector<unsigned char> &&encryptedSeedWithCS,
                    std::vector<unsigned char> &&encryptedGrinboxAddress)
        : m_pWallet(pWallet), m_encryptedSeedWithCS(std::move(encryptedSeedWithCS)),
          m_encryptedGrinboxAddress(std::move(encryptedGrinboxAddress))
    {
    }

    ~LoggedInSession()
    {
        delete m_pWallet;
    }

    std::mutex m_mutex;
    Wallet *m_pWallet;
    std::vector<unsigned char> m_encryptedSeedWithCS;
    std::vector<unsigned char> m_encryptedGrinboxAddress;
};