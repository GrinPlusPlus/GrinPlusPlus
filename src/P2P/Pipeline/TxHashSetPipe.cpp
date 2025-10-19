#include "TxHashSetPipe.h"
#include "../Connection.h"
#include "../ConnectionManager.h"
#include "../Messages/TxHashSetArchiveMessage.h"

#include <Common/Util/HexUtil.h>
#include <Common/Util/FileUtil.h>
#include <Common/Util/ThreadUtil.h>
#include <Common/Logger.h>
#include <Core/File/FileRemover.h>
#include <Core/Global.h>
#include <BlockChain/BlockChain.h>

#include <filesystem.h>
#include <fstream>

static const int BUFFER_SIZE = 8192 * 8; // between 8kB and 64kB should be OK

TxHashSetPipe::~TxHashSetPipe()
{
	ThreadUtil::Join(m_txHashSetThread);
	ThreadUtil::JoinAll(m_sendThreads);
}

std::shared_ptr<TxHashSetPipe> TxHashSetPipe::Create(
	const ConnectionManagerPtr& pConnectionManager,
	const IBlockChain::Ptr& pBlockChain,
	SyncStatusPtr pSyncStatus)
{
	return std::shared_ptr<TxHashSetPipe>(new TxHashSetPipe(
		pConnectionManager,
		pBlockChain,
		pSyncStatus
	));
}

void TxHashSetPipe::ReceiveTxHashSet(const Connection::Ptr& pConnection, const TxHashSetArchiveMessage& archive_msg)
{
	if (m_pSyncStatus->GetStatus() != ESyncStatus::SYNCING_TXHASHSET)
	{
		LOG_WARNING_F("Received TxHashSet from {} when not requested.", *pConnection);
		// connection.BanPeer(EBanReason::Abusive);
		return;
	}

	const bool processing = m_processing.exchange(true);
	if (processing) {
		LOG_WARNING_F("Received TxHashSet from {} when already processing another.", *pConnection);
		// pConnection->BanPeer(EBanReason::Abusive);
		return;
	}

	ThreadUtil::Join(m_txHashSetThread);

	m_txHashSetThread = std::thread(
		Thread_ProcessTxHashSet,
		std::ref(*this),
		pConnection,
		archive_msg.GetZippedSize(),
		archive_msg.GetBlockHash()
	);
}

void TxHashSetPipe::Thread_ProcessTxHashSet(TxHashSetPipe& pipeline, Connection::Ptr pConnection, const uint64_t zipped_size, const Hash blockHash)
{
	const std::string fileName = StringUtil::Format(
		"txhashset_{}.zip",
		HASH::ShortHash(blockHash)
	);
	const fs::path txHashSetPath = fs::temp_directory_path() / fileName;

	LOG_TRACE_F("Disabling receives and sends for {}", pConnection);
	pConnection->DisableReceives(true);
	pConnection->DisableSends(true);
	try
	{
		LoggerAPI::SetThreadName("TXHASHSET_PIPE");
		LOG_TRACE("BEGIN");

		LOG_INFO_F("Downloading TxHashSet from {}", *pConnection);

		std::ofstream fout;
		fout.open(txHashSetPath, std::ios::binary | std::ios::out | std::ios::trunc);
		
		size_t bytesReceived = 0;
		std::vector<uint8_t> buffer(BUFFER_SIZE, 0);

		pConnection->IsBusy(true);

		LOG_TRACE_F("Setting Receiving Buffer Size for {}", pConnection);
		if (zipped_size > BUFFER_SIZE)
		{
			pConnection->GetSocket()->SetReceiveBufferSize(BUFFER_SIZE);
		}
		else {
			pConnection->GetSocket()->SetReceiveBufferSize(zipped_size);
		}

		pipeline.m_pSyncStatus->UpdateDownloaded(0);
		pipeline.m_pSyncStatus->UpdateDownloadSize(zipped_size);

		bool received = false; // we will check this after the next loop

		for(;;)
		{
			if (!Global::IsRunning()) return;

			const int bufferSize = (std::min)((int)(zipped_size - bytesReceived), BUFFER_SIZE);
			received = pConnection->ReceiveSync(buffer, bufferSize);

			if (received)
			{
				fout.write((char*)&buffer[0], bufferSize);
				bytesReceived += bufferSize;

				pipeline.m_pSyncStatus->UpdateDownloaded(bytesReceived);
				
				if (bytesReceived == zipped_size)
				{
					LOG_INFO("Closing file...");
					fout.flush();
					fout.close();
					break;
				}

				continue;
			}

			break;
		}
		// Going back to normal
		LOG_INFO("Closing file...");
		fout.flush();
		fout.close();

		pConnection->IsBusy(false);
		LOG_TRACE_F("Setting Receiving Buffer Size for {} to default size", pConnection);
		pConnection->GetSocket()->SetDefaults();
		LOG_INFO_F("Enabling receives and sends for {}", pConnection);
		pConnection->DisableReceives(false);
		pConnection->DisableSends(false);
		
		if (!received)
		{
			LOG_ERROR("Transmission ended abruptly");

			FileUtil::RemoveFile(txHashSetPath);

			pipeline.m_processing = false;
			pipeline.m_pSyncStatus->UpdateStatus(ESyncStatus::TXHASHSET_SYNC_FAILED);
			// pConnection->BanPeer(EBanReason::BadTxHashSet);

			return;
		}

		LOG_INFO("Downloading successful");

		SyncStatusPtr pSyncStatus = pipeline.m_pSyncStatus;

		pSyncStatus->UpdateProcessingStatus(0);
		pSyncStatus->UpdateStatus(ESyncStatus::PROCESSING_TXHASHSET);

		EBlockChainStatus processStatus = pipeline.m_pBlockChain->ProcessTransactionHashSet(blockHash, txHashSetPath, *pSyncStatus);
		if (processStatus == EBlockChainStatus::INVALID)
		{
			LOG_ERROR("Invalid TxHashSet received.");
			pSyncStatus->UpdateStatus(ESyncStatus::TXHASHSET_SYNC_FAILED);
			//pConnection->BanPeer(EBanReason::BadTxHashSet);
		}
		else
		{
			pSyncStatus->UpdateStatus(ESyncStatus::SYNCING_BLOCKS);
		}

		LOG_TRACE("END");
	}
	catch (...)
	{
		LOG_ERROR_F("Exception thrown while downloading/processing TxHashSet from {}", *pConnection);
		pipeline.m_pSyncStatus->UpdateStatus(ESyncStatus::TXHASHSET_SYNC_FAILED);
		// pConnection->BanPeer(EBanReason::BadTxHashSet);
		FileUtil::RemoveFile(txHashSetPath);
	}

	pipeline.m_processing = false;
}

void TxHashSetPipe::SendTxHashSet(const std::shared_ptr<Connection>& pConnection, const Hash& block_hash)
{
	const time_t maxTxHashSetRequest = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() - std::chrono::hours(2));
	if (pConnection->GetPeer()->GetLastTxHashSetRequest() > maxTxHashSetRequest) {
		LOG_WARNING_F("Peer '{}' requested multiple TxHashSet's within 2 hours.", pConnection->GetIPAddress());
		pConnection->BanPeer(EBanReason::Abusive);
		return;
	}

	LOG_INFO_F("Sending TxHashSet snapshot to {}", pConnection);
	pConnection->GetPeer()->UpdateLastTxHashSetRequest();

	std::thread send_thread = std::thread(
		Thread_SendTxHashSet,
		m_pBlockChain,
		pConnection,
		block_hash
	);
	m_sendThreads.push_back(std::move(send_thread));
}

void TxHashSetPipe::Thread_SendTxHashSet(
	IBlockChain::Ptr pBlockChain,
	std::shared_ptr<Connection> pConnection,
	Hash block_hash)
{

	auto pHeader = pBlockChain->GetBlockHeaderByHash(block_hash);
	if (pHeader == nullptr) {
		return;
	}

	fs::path zipFilePath;

	try {
		zipFilePath = pBlockChain->SnapshotTxHashSet(pHeader);
	}
	catch (std::exception&) {
		return;
	}

	pConnection->DisableSends(true);

	// Hack until I can determine why zips aren't deleted.
	FileRemover remover(zipFilePath);

	std::ifstream file(zipFilePath, std::ios::in | std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		FileUtil::RemoveFile(zipFilePath);
		return;
	}

	try {
		const uint64_t fileSize = FileUtil::GetFileSize(zipFilePath);
		file.seekg(0);

		pConnection->SendSync(TxHashSetArchiveMessage{ pHeader->GetHash(), pHeader->GetHeight(), fileSize });

		std::vector<uint8_t> buffer(BUFFER_SIZE, 0);
			
		while (file.read((char*)buffer.data(), BUFFER_SIZE)) {
			std::vector<uint8_t> bytesToSend(
				buffer.cbegin(),
				buffer.cbegin() + file.gcount()
			);
			bool sent = pConnection->GetSocket()->SendSync(bytesToSend, false);
			if (!sent || !Global::IsRunning()) {
				throw std::runtime_error("Transmission ended abruptly");
			}
		}

		pConnection->DisableSends(false);
	}
	catch (std::exception& e) {
		LOG_ERROR_F("Exception thrown while sending TxHashSet: {}", e.what());
	}

	file.close();
	FileUtil::RemoveFile(zipFilePath);
}