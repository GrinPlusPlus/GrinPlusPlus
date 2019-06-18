#pragma once

#include "../ChainState.h"

#include <PMMR/TxHashSet.h>
#include <Config/Config.h>
#include <Crypto/Hash.h>
#include <P2P/SyncStatus.h>
#include <string>

// Forward Declarations
class IBlockChainServer;
class BlockHeader;
class IBlockDB;

class TxHashSetProcessor
{
public:
	TxHashSetProcessor(const Config& config, IBlockChainServer& blockChainServer, ChainState& chainState, IBlockDB& blockDB);

	bool ProcessTxHashSet(const Hash& blockHash, const std::string& path, SyncStatus& syncStatus);

private:
	bool UpdateConfirmedChain(LockedChainState& lockedState, const BlockHeader& blockHeader);

	const Config& m_config;
	IBlockChainServer& m_blockChainServer;
	ChainState& m_chainState;
	IBlockDB& m_blockDB;
};