#include "Syncer.h"
#include "HeaderSyncer.h"
#include "StateSyncer.h"
#include "BlockSyncer.h"

#include <BlockChain/BlockChainServer.h>
#include <P2P/SyncStatus.h>
#include <Common/ThreadManager.h>
#include <Common/Logger.h>
#include <Common/Util/ThreadUtil.h>

static const int MINIMUM_NUM_PEERS = 4;

Syncer::Syncer(
	std::weak_ptr<ConnectionManager> pConnectionManager,
	IBlockChainServerPtr pBlockChainServer,
	std::shared_ptr<Pipeline> pPipeline,
	SyncStatusPtr pSyncStatus)
	: m_pConnectionManager(pConnectionManager),
	m_pBlockChainServer(pBlockChainServer),
	m_pPipeline(pPipeline),
	m_pSyncStatus(pSyncStatus),
	m_terminate(false)
{

}

Syncer::~Syncer()
{
	m_terminate = true;
	ThreadUtil::Join(m_syncThread);
}

std::shared_ptr<Syncer> Syncer::Create(
	std::weak_ptr<ConnectionManager> pConnectionManager,
	IBlockChainServerPtr pBlockChainServer,
	std::shared_ptr<Pipeline> pPipeline,
	SyncStatusPtr pSyncStatus)
{
	std::shared_ptr<Syncer> pSyncer = std::shared_ptr<Syncer>(new Syncer(
		pConnectionManager,
		pBlockChainServer,
		pPipeline,
		pSyncStatus
	));
	pSyncer->m_syncThread = std::thread(Thread_Sync, std::ref(*pSyncer));

	return pSyncer;
}

void Syncer::Thread_Sync(Syncer& syncer)
{
	ThreadManagerAPI::SetCurrentThreadName("SYNC");
	LOG_DEBUG("BEGIN");

	HeaderSyncer headerSyncer(syncer.m_pConnectionManager, syncer.m_pBlockChainServer);
	StateSyncer stateSyncer(syncer.m_pConnectionManager, syncer.m_pBlockChainServer);
	BlockSyncer blockSyncer(syncer.m_pConnectionManager, syncer.m_pBlockChainServer, syncer.m_pPipeline);
	bool startup = true;

	while (!syncer.m_terminate)
	{
		try
		{
			ThreadUtil::SleepFor(std::chrono::milliseconds(10), syncer.m_terminate);
			syncer.UpdateSyncStatus();

			if (syncer.m_pSyncStatus->GetNumActiveConnections() >= MINIMUM_NUM_PEERS)
			{
				// Sync Headers
				if (headerSyncer.SyncHeaders(*syncer.m_pSyncStatus, startup))
				{
					if (syncer.m_pSyncStatus->GetStatus() != ESyncStatus::SYNCING_TXHASHSET && syncer.m_pSyncStatus->GetStatus() != ESyncStatus::PROCESSING_TXHASHSET)
					{
						syncer.m_pSyncStatus->UpdateStatus(ESyncStatus::SYNCING_HEADERS);
					}
					continue;
				}

				// Sync State (TxHashSet)
				if (stateSyncer.SyncState(*syncer.m_pSyncStatus))
				{
					continue;
				}

				// Sync Blocks
				if (blockSyncer.SyncBlocks(*syncer.m_pSyncStatus, startup))
				{
					syncer.m_pSyncStatus->UpdateStatus(ESyncStatus::SYNCING_BLOCKS);
					continue;
				}

				startup = false;

				syncer.m_pSyncStatus->UpdateStatus(ESyncStatus::NOT_SYNCING);
			}
			else if (syncer.m_pSyncStatus->GetStatus() != ESyncStatus::PROCESSING_TXHASHSET)
			{
				syncer.m_pSyncStatus->UpdateStatus(ESyncStatus::WAITING_FOR_PEERS);
			}
		}
		catch (std::exception& e)
		{
			LOG_ERROR_F("Exception thrown: {}", e);
		}
	}

	LOG_DEBUG("END");
}

void Syncer::UpdateSyncStatus()
{
	m_pBlockChainServer->UpdateSyncStatus(*m_pSyncStatus);
	m_pConnectionManager.lock()->UpdateSyncStatus(*m_pSyncStatus);
}