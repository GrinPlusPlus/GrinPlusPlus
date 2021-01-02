#include "TxHashSetPipe.h"
#include "../Connection.h"
#include "../ConnectionManager.h"
#include "../Messages/TxHashSetArchiveMessage.h"

#include <Common/Util/HexUtil.h>
#include <Common/Util/FileUtil.h>
#include <Common/Util/ThreadUtil.h>
#include <Common/Logger.h>
#include <Core/Global.h>
#include <BlockChain/BlockChain.h>

#include <filesystem.h>
#include <fstream>

static const int BUFFER_SIZE = 256 * 1024;

TxHashSetPipe::~TxHashSetPipe()
{
	ThreadUtil::Join(m_txHashSetThread);
}

std::shared_ptr<TxHashSetPipe> TxHashSetPipe::Create(
	const Config& config,
	const IBlockChain::Ptr& pBlockChain,
	SyncStatusPtr pSyncStatus)
{
	return std::shared_ptr<TxHashSetPipe>(new TxHashSetPipe(
		config,
		pBlockChain,
		pSyncStatus
	));
}

void TxHashSetPipe::ReceiveTxHashSet(Connection& connection, const TxHashSetArchiveMessage& txHashSetArchiveMessage)
{
	if (m_pSyncStatus->GetStatus() != ESyncStatus::SYNCING_TXHASHSET)
	{
		LOG_WARNING_F("Received TxHashSet from {} when not requested.", connection);
		// connection.BanPeer(EBanReason::Abusive);
		// return;
	}

	const bool processing = m_processing.exchange(true);
	if (processing) {
		LOG_WARNING_F("Received TxHashSet from {} when already processing another.", connection);
		connection.BanPeer(EBanReason::Abusive);
		return;
	}

	LOG_INFO_F("Downloading TxHashSet from {}", connection);

	m_pSyncStatus->UpdateDownloaded(0);
	m_pSyncStatus->UpdateDownloadSize(txHashSetArchiveMessage.GetZippedSize());

	SocketPtr pSocket = connection.GetSocket();
	pSocket->SetReceiveTimeout(10 * 1000);
	pSocket->SetReceiveBufferSize(BUFFER_SIZE);

	const std::string fileName = StringUtil::Format(
		"txhashset_{}.zip",
		HASH::ShortHash(txHashSetArchiveMessage.GetBlockHash())
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

			const bool received = pSocket->Receive(bytesToRead, false, Socket::BLOCKING, buffer);
			if (!received || !Global::IsRunning())
			{
				LOG_ERROR("Transmission ended abruptly");
				fout.close();
				FileUtil::RemoveFile(txHashSetPath);
				m_processing = false;
				m_pSyncStatus->UpdateStatus(ESyncStatus::TXHASHSET_SYNC_FAILED);
				connection.BanPeer(EBanReason::BadTxHashSet);

				return;
			}

			fout.write((char*)&buffer[0], bytesToRead);
			bytesReceived += bytesToRead;

			m_pSyncStatus->UpdateDownloaded(bytesReceived);
		}

		fout.close();
	}
	catch (...)
	{
		LOG_ERROR_F("Exception thrown while downloading TxHashSet from {}", connection);
		m_processing = false;
		m_pSyncStatus->UpdateStatus(ESyncStatus::TXHASHSET_SYNC_FAILED);
		connection.BanPeer(EBanReason::BadTxHashSet);

		throw;
	}

	LOG_INFO("Downloading successful");

	ThreadUtil::Join(m_txHashSetThread);

	m_txHashSetThread = std::thread(
		Thread_ProcessTxHashSet,
		std::ref(*this),
		connection.GetPeer(),
		txHashSetArchiveMessage.GetBlockHash(),
		txHashSetPath.u8string()
	);
}

void TxHashSetPipe::Thread_ProcessTxHashSet(TxHashSetPipe& pipeline, PeerPtr pPeer, const Hash blockHash, const fs::path path)
{
	try
	{
		LoggerAPI::SetThreadName("TXHASHSET_PIPE");
		LOG_TRACE("BEGIN");

		SyncStatusPtr pSyncStatus = pipeline.m_pSyncStatus;

		pSyncStatus->UpdateProcessingStatus(0);
		pSyncStatus->UpdateStatus(ESyncStatus::PROCESSING_TXHASHSET);

		const EBlockChainStatus processStatus = pipeline.m_pBlockChain->ProcessTransactionHashSet(blockHash, path, *pSyncStatus);
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
