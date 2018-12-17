#pragma once

#include "Chain.h"

#include <Config/Config.h>

// TODO: Move to Database
class ChainStore
{
public:
	ChainStore(const Config& config, BlockIndex* pGenesisIndex);
	bool Load();
	bool Flush();

	Chain& GetChain(const EChainType chainType);
	BlockIndex* GetOrCreateIndex(const Hash& hash, const uint64_t height, BlockIndex* pPreviousIndex);
	BlockIndex* FindCommonIndex(const EChainType chainType1, const EChainType chainType2);

	inline Chain& GetConfirmedChain() { return m_confirmedChain; }
	inline Chain& GetCandidateChain() { return m_candidateChain; }
	inline Chain& GetSyncChain() { return m_syncChain; }

private:
	bool ReadChain(Chain& chain, const std::string& path);
	bool WriteChain(Chain& chain, const std::string& path);

	bool m_loaded;
	Chain m_confirmedChain;
	Chain m_candidateChain;
	Chain m_syncChain;

	const Config& m_config;
};