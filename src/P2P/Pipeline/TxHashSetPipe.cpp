#include "TxHashSetPipe.h"
#include "../ConnectionManager.h"
#include "../Messages/TxHashSetArchiveMessage.h"

#include <Common/Util/HexUtil.h>
#include <Common/Util/FileUtil.h>
#include <Common/Util/ThreadUtil.h>
#include <Infrastructure/ShutdownManager.h>
#include <Infrastructure/ThreadManager.h>
#include <Infrastructure/Logger.h>
#include <BlockChain/BlockChainServer.h>

static const int BUFFER_SIZE = 256 * 1024;

TxHashSetPipe::TxHashSetPipe(
	const Config& config,
	IBlockChainServerPtr pBlockChainServer,
	SyncStatusPtr pSyncStatus)
	: m_config(config),
	m_pBlockChainServer(pBlockChainServer),
	m_pSyncStatus(pSyncStatus),
	m_processing(false)
{

}

TxHashSetPipe::~TxHashSetPipe()
{
	ThreadUtil::Join(m_txHashSetThread);
}

std::shared_ptr<TxHashSetPipe> TxHashSetPipe::Create(
	const Config& config,
	IBlockChainServerPtr pBlockChainServer,
	SyncStatusPtr pSyncStatus)
{
	return std::shared_ptr<TxHashSetPipe>(new TxHashSetPipe(
		config,
		pBlockChainServer,
		pSyncStatus
	));
}

bool TxHashSetPipe::ReceiveTxHashSet(PeerPtr pPeer, Socket& socket, const TxHashSetArchiveMessage& txHashSetArchiveMessage)
{
	if (m_pSyncStatus->GetStatus() != ESyncStatus::SYNCING_TXHASHSET)
	{
		LOG_WARNING_F("Received TxHashSet from Peer ({}) when not requested.", pPeer);
		//return false;
	}

	const bool processing = m_processing.exchange(true);
	if (processing)
	{
		LOG_WARNING_F("Received TxHashSet from Peer ({}) when already processing another.", pPeer);
		return false;
	}

	LOG_INFO_F("Downloading TxHashSet from {}", pPeer);

	m_pSyncStatus->UpdateDownloaded(0);
	m_pSyncStatus->UpdateDownloadSize(txHashSetArchiveMessage.GetZippedSize());

	socket.SetReceiveTimeout(10 * 1000);
	socket.SetReceiveBufferSize(BUFFER_SIZE);

	const std::string fileName = StringUtil::Format(
		"txhashset_{}.zip",
		HexUtil::ShortHash(txHashSetArchiveMessage.GetBlockHash())
	);
	const fs::path txHashSetPath =  fs::temp_directory_path() / fileName;

	try
	{
		std::ofstream fout;
		fout.open(txHashSetPath, std::ios::binary | std::ios::out | std::ios::trunc);

		size_t bytesReceived = 0;
		std::vector<unsigned char> buffer(BUFFER_SIZE, 0);
		while (bytesReceived < txHashSetArchiveMessage.GetZippedSize())
		{
			const int bytesToRead = (std::min)((int)(txHashSetArchiveMessage.GetZippedSize() - bytesReceived), BUFFER_SIZE);

			const bool received = socket.Receive(bytesToRead, false, buffer);
			if (!received || ShutdownManagerAPI::WasShutdownRequested())
			{
				LOG_ERROR("Transmission ended abruptly");
				fout.close();
				FileUtil::RemoveFile(txHashSetPath.u8string());
				m_processing = false;
				m_pSyncStatus->UpdateStatus(ESyncStatus::TXHASHSET_SYNC_FAILED);

				return false;
			}

			fout.write((char*)&buffer[0], bytesToRead);
			bytesReceived += bytesToRead;

			m_pSyncStatus->UpdateDownloaded(bytesReceived);
		}

		fout.close();
	}
	catch (...)
	{
		LOG_ERROR_F("Exception thrown while downloading TxHashSet from {}", *pPeer);
		m_processing = false;
		m_pSyncStatus->UpdateStatus(ESyncStatus::TXHASHSET_SYNC_FAILED);
		throw;
	}

	LOG_INFO("Downloading successful");

	ThreadUtil::Join(m_txHashSetThread);

	m_txHashSetThread = std::thread(Thread_ProcessTxHashSet, std::ref(*this), pPeer, txHashSetArchiveMessage.GetBlockHash(), txHashSetPath.u8string());

	return true;
}

void TxHashSetPipe::Thread_ProcessTxHashSet(TxHashSetPipe& pipeline, PeerPtr pPeer, const Hash blockHash, const std::string path)
{
	try
	{
		ThreadManagerAPI::SetCurrentThreadName("TXHASHSET_PIPE");
		LOG_TRACE("BEGIN");

		SyncStatusPtr pSyncStatus = pipeline.m_pSyncStatus;

		pSyncStatus->UpdateProcessingStatus(0);
		pSyncStatus->UpdateStatus(ESyncStatus::PROCESSING_TXHASHSET);

		const EBlockChainStatus processStatus = pipeline.m_pBlockChainServer->ProcessTransactionHashSet(blockHash, path, *pSyncStatus);
		if (processStatus == EBlockChainStatus::INVALID)
		{
			LOG_ERROR("Invalid TxHashSet received.");
			pSyncStatus->UpdateStatus(ESyncStatus::TXHASHSET_SYNC_FAILED);
			pPeer->Ban(EBanReason::BadTxHashSet);
		}
		else
		{
			pSyncStatus->UpdateStatus(ESyncStatus::SYNCING_BLOCKS);
		}

		LOG_TRACE("END");
	}
	catch (...)
	{
		pPeer->Ban(EBanReason::BadTxHashSet);
		LOG_ERROR("Exception thrown in thread.");
	}

	pipeline.m_processing = false;
}