#include "Syncer.h"
#include "HeaderSyncer.h"
#include "StateSyncer.h"
#include "BlockSyncer.h"

#include <BlockChain/BlockChain.h>
#include <P2P/SyncStatus.h>
#include <Common/Logger.h>
#include <Common/Util/ThreadUtil.h>
#include <Core/Global.h>

Syncer::Syncer(
    std::weak_ptr<ConnectionManager> pConnectionManager,
    const IBlockChain::Ptr& pBlockChain,
    std::shared_ptr<Pipeline> pPipeline,
    SyncStatusPtr pSyncStatus)
    : m_pConnectionManager(pConnectionManager),
    m_pBlockChain(pBlockChain),
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
    const IBlockChain::Ptr& pBlockChain,
    std::shared_ptr<Pipeline> pPipeline,
    SyncStatusPtr pSyncStatus)
{
    std::shared_ptr<Syncer> pSyncer = std::shared_ptr<Syncer>(new Syncer(
        pConnectionManager,
        pBlockChain,
        pPipeline,
        pSyncStatus
    ));
    pSyncer->m_syncThread = std::thread(Thread_Sync, std::ref(*pSyncer));

    return pSyncer;
}

void Syncer::Thread_Sync(Syncer& syncer)
{
    LoggerAPI::SetThreadName("SYNC");
    LOG_DEBUG("BEGIN");

    HeaderSyncer headerSyncer(syncer.m_pConnectionManager, syncer.m_pBlockChain);
    StateSyncer stateSyncer(syncer.m_pConnectionManager, syncer.m_pBlockChain);
    BlockSyncer blockSyncer(syncer.m_pConnectionManager, syncer.m_pBlockChain, syncer.m_pPipeline);
    bool headers_synced = false;
    bool blocks_synced = false;
    auto pStatus = syncer.m_pSyncStatus;

    while (!syncer.m_terminate && Global::IsRunning()) {
        try {
            ThreadUtil::SleepFor(std::chrono::milliseconds(10));
            syncer.UpdateSyncStatus();

            if (pStatus->GetNumActiveConnections() >= Global::GetConfig().GetMinSyncPeers()) {
                // Sync Headers
                if (headerSyncer.SyncHeaders(*pStatus, !headers_synced)) {
                    if (pStatus->GetStatus() != ESyncStatus::SYNCING_TXHASHSET && pStatus->GetStatus() != ESyncStatus::PROCESSING_TXHASHSET) {
                        pStatus->UpdateStatus(ESyncStatus::SYNCING_HEADERS);
                    }
                    continue;
                } else {
                    headers_synced = true;
                }

                // Sync State (TxHashSet)
                if (stateSyncer.SyncState(*pStatus)) {
                    continue;
                }

                // Sync Blocks
                if (blockSyncer.SyncBlocks(*pStatus, !blocks_synced)) {
                    pStatus->UpdateStatus(ESyncStatus::SYNCING_BLOCKS);
                    continue;
                } else {
                    blocks_synced = true;
                }

                pStatus->UpdateStatus(ESyncStatus::NOT_SYNCING);
            } else if (pStatus->GetStatus() != ESyncStatus::PROCESSING_TXHASHSET) {
                pStatus->UpdateStatus(ESyncStatus::WAITING_FOR_PEERS);
            }
        }
        catch (std::exception& e) {
            LOG_ERROR_F("Exception thrown: {}", e);
        }
    }

    LOG_DEBUG("END");
}

void Syncer::UpdateSyncStatus()
{
    m_pBlockChain->UpdateSyncStatus(*m_pSyncStatus);
    m_pConnectionManager.lock()->UpdateSyncStatus(*m_pSyncStatus);
}