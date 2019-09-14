#pragma once

#include "Chain.h"

#include <Config/Config.h>

// TODO: Use memory-mapped file
class ChainStore
{
public:
	ChainStore(const Config& config, BlockIndex* pGenesisIndex);
	bool Load();
	bool Flush();

	Chain& GetChain(const EChainType chainType);
	const Chain& GetChain(const EChainType chainType) const;
	BlockIndex* GetOrCreateIndex(const Hash& hash, const uint64_t height, BlockIndex* pPreviousIndex);
	BlockIndex* FindCommonIndex(const EChainType chainType1, const EChainType chainType2);

	//
	// Applies all of the blocks from the source chain to the destination chain, up to the specified height.
	//
	bool ReorgChain(const EChainType source, const EChainType destination, const uint64_t height);

	bool AddBlock(const EChainType source, const EChainType destination, const uint64_t height);

	inline Chain& GetConfirmedChain() { return m_confirmedChain; }
	inline Chain& GetCandidateChain() { return m_candidateChain; }
	inline Chain& GetSyncChain() { return m_syncChain; }

private:
	bool m_loaded;
	Chain m_confirmedChain;
	Chain m_candidateChain;
	Chain m_syncChain;

	const Config& m_config;
};