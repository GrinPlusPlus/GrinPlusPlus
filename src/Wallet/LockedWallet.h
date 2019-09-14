#pragma once

#include "Wallet.h"
#include <mutex>

class LockedWallet
{
public:
	LockedWallet(std::mutex& mutex, Wallet& wallet)
		: m_pReferences(new int(1)),
		m_mutex(mutex),
		m_wallet(wallet)
	{
		m_mutex.lock();
	}

	~LockedWallet()
	{
		if (--(*m_pReferences) == 0)
		{
			m_mutex.unlock();
			delete m_pReferences;
		}
	}

	LockedWallet(const LockedWallet& other)
		: m_pReferences(other.m_pReferences),
		m_mutex(other.m_mutex),
		m_wallet(other.m_wallet)
	{
		++(*m_pReferences);
	}
	LockedWallet& operator=(const LockedWallet&) = delete;

	inline Wallet& GetWallet() { return m_wallet; }

private:
	int* m_pReferences;
	std::mutex& m_mutex;
	Wallet& m_wallet;
};