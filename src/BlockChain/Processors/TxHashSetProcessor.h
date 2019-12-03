#pragma once

#include "../ChainState.h"

#include <PMMR/TxHashSet.h>
#include <Config/Config.h>
#include <Crypto/Hash.h>
#include <P2P/SyncStatus.h>
#include <filesystem.h>
#include <string>

// Forward Declarations
class IBlockChainServer;
class BlockHeader;
class IBlockDB;

class TxHashSetProcessor
{
public:
	TxHashSetProcessor(const Config& config, IBlockChainServer& blockChainServer, std::shared_ptr<Locked<ChainState>> pChainState);

	bool ProcessTxHashSet(const Hash& blockHash, const fs::path& path, SyncStatus& syncStatus);

private:
	bool UpdateConfirmedChain(Writer<ChainState> pLockedState, const BlockHeader& blockHeader);

	const Config& m_config;
	IBlockChainServer& m_blockChainServer;
	std::shared_ptr<Locked<ChainState>> m_pChainState;
};