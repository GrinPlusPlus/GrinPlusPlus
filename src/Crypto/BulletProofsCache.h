#pragma once

#include <caches/Cache.h>
#include <Crypto/Models/Commitment.h>
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

		m_bulletproofsCache.Put(commitment, commitment);
	}

	bool WasAlreadyVerified(const Commitment& commitment) const
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		if (m_bulletproofsCache.Cached(commitment))
		{
			return m_bulletproofsCache.Get(commitment) == commitment;
		}

		return false;
	}

private:
	mutable std::mutex m_mutex;
	mutable LRUCache<Commitment, Commitment> m_bulletproofsCache;
};