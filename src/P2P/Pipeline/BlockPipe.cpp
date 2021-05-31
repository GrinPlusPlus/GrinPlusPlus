#include "BlockPipe.h"
#include "../Messages/TransactionKernelMessage.h"
#include "../ConnectionManager.h"

#include <Common/Util/ThreadUtil.h>
#include <Common/Logger.h>
#include <BlockChain/BlockChain.h>

BlockPipe::BlockPipe(const Config& config, const IBlockChain::Ptr& pBlockChain)
	: m_config(config), m_pBlockChain(pBlockChain), m_terminate(false)
{
}

BlockPipe::~BlockPipe()
{
	m_terminate = true;

	ThreadUtil::Join(m_blockThread);
	ThreadUtil::Join(m_processThread);
}

std::shared_ptr<BlockPipe> BlockPipe::Create(const Config& config, const IBlockChain::Ptr& pBlockChain)
{
	std::shared_ptr<BlockPipe> pBlockPipe = std::shared_ptr<BlockPipe>(new BlockPipe(config, pBlockChain));
	pBlockPipe->m_blockThread = std::thread(Thread_ProcessNewBlocks, std::ref(*pBlockPipe.get()));
	pBlockPipe->m_processThread = std::thread(Thread_PostProcessBlocks, std::ref(*pBlockPipe.get()));

	return pBlockPipe;
}

void BlockPipe::Thread_ProcessNewBlocks(BlockPipe& pipeline)
{
	LoggerAPI::SetThreadName("BLOCK_PREPROCESS_PIPE");
	LOG_TRACE("BEGIN");

	while (!pipeline.m_terminate && Global::IsRunning())
	{
		std::vector<BlockEntry> blocksToProcess = pipeline.m_blocksToProcess.copy_front(8); // TODO: Use number of CPU threads.
		if (!blocksToProcess.empty())
		{
			if (blocksToProcess.size() == 1)
			{
				ProcessNewBlock(pipeline, blocksToProcess.front());
			}
			else
			{
				std::vector<std::thread> tasks;
				for (const BlockEntry& blockEntry : blocksToProcess)
				{
					tasks.push_back(std::thread([&pipeline, blockEntry] { ProcessNewBlock(pipeline, blockEntry); }));
				}

				ThreadUtil::JoinAll(tasks);
			}

			pipeline.m_blocksToProcess.pop_front(blocksToProcess.size());
		}
		else
		{
			ThreadUtil::SleepFor(std::chrono::milliseconds(5));
		}
	}

	LOG_TRACE("END");
}

void BlockPipe::ProcessNewBlock(BlockPipe& pipeline, const BlockEntry& blockEntry)
{
	try
	{
		const EBlockChainStatus status = pipeline.m_pBlockChain->AddBlock(blockEntry.m_block);
		if (status == EBlockChainStatus::INVALID)
		{
			blockEntry.m_peer->Ban(EBanReason::BadBlock);
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception ({}) caught while attempting to add block {}.", e.what(), blockEntry.m_block);
		blockEntry.m_peer->Ban(EBanReason::BadBlock);
	}
}

void BlockPipe::Thread_PostProcessBlocks(BlockPipe& pipeline)
{
	LoggerAPI::SetThreadName("BLOCK_POSTPROCESS_PIPE");
	LOG_TRACE("BEGIN");

	while (!pipeline.m_terminate && Global::IsRunning())
	{
		if (!pipeline.m_pBlockChain->ProcessNextOrphanBlock())
		{
			ThreadUtil::SleepFor(std::chrono::milliseconds(5));
		}
	}

	LOG_TRACE("END");
}

bool BlockPipe::AddBlockToProcess(PeerPtr pPeer, const FullBlock& block)
{
	std::function<bool(const BlockEntry&, const BlockEntry&)> comparator = [](const BlockEntry& blockEntry1, const BlockEntry& blockEntry2)
	{
		return blockEntry1.m_block.GetHash() == blockEntry2.m_block.GetHash();
	};

	return m_blocksToProcess.push_back_unique(BlockEntry(pPeer, block), comparator);
}

bool BlockPipe::IsProcessingBlock(const Hash& hash) const
{
	std::function<bool(const BlockEntry&, const Hash&)> comparator = [](const BlockEntry& blockEntry, const Hash& hash)
	{
		return blockEntry.m_block.GetHash() == hash;
	};

	return m_blocksToProcess.contains<Hash>(hash, comparator) || m_pBlockChain->HasOrphan(hash);
}