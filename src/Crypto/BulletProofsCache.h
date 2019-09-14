#pragma once

#include <lru/cache.hpp>
#include <Crypto/Commitment.h>
#include <mutex>

class BulletProofsCache
{
public:
	BulletProofsCache()
		: m_bulletproofsCache(3000)
	{

	}

	void AddToCache(const Commitment& commitment)
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		m_bulletproofsCache.insert(commitment, commitment);
	}

	bool WasAlreadyVerified(const Commitment& commitment) const
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		auto iter = m_bulletproofsCache.find(commitment);
		if (iter != m_bulletproofsCache.end() && iter->value() == commitment)
		{
			return true;
		}

		return false;
	}

private:
	mutable std::mutex m_mutex;
	mutable LRU::Cache<Commitment, Commitment> m_bulletproofsCache;
};