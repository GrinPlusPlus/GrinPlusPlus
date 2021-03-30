#pragma once

#include <Crypto/Models/Hash.h>
#include <P2P/Peer.h>
#include <Core/Models/FullBlock.h>
#include <BlockChain/BlockChain.h>
#include <Common/ConcurrentQueue.h>
#include <string>
#include <cstdint>
#include <atomic>
#include <thread>

// Forward Declarations
class Config;
class TxHashSetArchiveMessage;
class Transaction;

class BlockPipe
{
public:
	static std::shared_ptr<BlockPipe> Create(
		const Config& config,
		const IBlockChain::Ptr& pBlockChain
	);
	~BlockPipe();

	bool AddBlockToProcess(PeerPtr pPeer, const FullBlock& block);
	bool IsProcessingBlock(const Hash& hash) const;

private:
	BlockPipe(const Config& config, const IBlockChain::Ptr& pBlockChain);

	const Config& m_config;
	IBlockChain::Ptr m_pBlockChain;

	struct BlockEntry
	{
		BlockEntry(PeerPtr pPeer, const FullBlock& fullBlock)
			: m_peer(pPeer), m_block(fullBlock)
		{

		}

		PeerPtr m_peer;
		FullBlock m_block;
	};

	// Pre-Process New Blocks
	static void Thread_ProcessNewBlocks(BlockPipe& pipeline);
	static void ProcessNewBlock(BlockPipe& pipeline, const BlockEntry& blockEntry);
	std::thread m_blockThread;
	ConcurrentQueue<BlockEntry> m_blocksToProcess;

	// Process Next Block
	std::thread m_processThread;
	static void Thread_PostProcessBlocks(BlockPipe& pipeline);

	std::atomic_bool m_terminate;
};