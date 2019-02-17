#pragma once

#include <Crypto/Hash.h>
#include <BlockChain/ChainType.h>
#include <Core/Models/BlockHeader.h>

class BlockIndex
{
public:
	BlockIndex(const Hash& hash, const uint64_t height, BlockIndex* pPrevious)
		: m_hash(hash), m_height(height), m_pPrevious(pPrevious), m_chainTypeMask(0)
	{

	}

	inline const Hash& GetHash() const { return m_hash; }
	inline uint64_t GetHeight() const { return m_height; }
	inline BlockIndex* GetPrevious() const { return m_pPrevious; }

	inline void AddChainType(const EChainType chainType) { m_chainTypeMask = ChainType::AddChainType(m_chainTypeMask, chainType); }
	inline void RemoveChainType(const EChainType chainType) { m_chainTypeMask = ChainType::RemoveChainType(m_chainTypeMask, chainType); }

	inline bool IsSync() const { return ChainType::IsSync(m_chainTypeMask); }
	inline bool IsCandidate() const { return ChainType::IsCandidate(m_chainTypeMask); }
	inline bool IsConfirmed() const { return ChainType::IsConfirmed(m_chainTypeMask); }
	inline bool IsInAllChains() const { return ChainType::IsAll(m_chainTypeMask); }
	inline bool IsSafeToDelete() const { return ChainType::IsNone(m_chainTypeMask); }

private:
	Hash m_hash;
	uint64_t m_height;
	BlockIndex* m_pPrevious; // TODO: Storing this is probably not necessary
	uint8_t m_chainTypeMask;
	// TODO: Also save output & kernel MMR sizes
};