#include "MessageProcessor.h"
#include "BlockLocator.h"
#include "ConnectionManager.h"
#include "Pipeline/Pipeline.h"

// Network Messages
#include "Messages/ErrorMessage.h"
#include "Messages/PingMessage.h"
#include "Messages/PongMessage.h"
#include "Messages/GetPeerAddressesMessage.h"
#include "Messages/PeerAddressesMessage.h"
#include "Messages/BanReasonMessage.h"

// BlockChain Messages
#include "Messages/GetHeadersMessage.h"
#include "Messages/HeaderMessage.h"
#include "Messages/HeadersMessage.h"
#include "Messages/BlockMessage.h"
#include "Messages/GetBlockMessage.h"
#include "Messages/CompactBlockMessage.h"
#include "Messages/GetCompactBlockMessage.h"

// Transaction Messages
#include "Messages/TransactionMessage.h"
#include "Messages/StemTransactionMessage.h"
#include "Messages/TxHashSetRequestMessage.h"
#include "Messages/TxHashSetArchiveMessage.h"
#include "Messages/GetTransactionMessage.h"
#include "Messages/TransactionKernelMessage.h"

#include <Core/File/FileRemover.h>
#include <Core/Exceptions/BadDataException.h>
#include <Core/Exceptions/BlockChainException.h>
#include <Core/Global.h>
#include <P2P/Common.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/StringUtil.h>
#include <Common/Util/FileUtil.h>
#include <BlockChain/BlockChain.h>
#include <Common/Logger.h>
#include <thread>
#include <fstream>

static const int BUFFER_SIZE = 256 * 1024;

using namespace MessageTypes;

MessageProcessor::MessageProcessor(
	const Config& config,
	ConnectionManager& connectionManager,
	Locked<PeerManager> peerManager,
	const IBlockChain::Ptr& pBlockChain,
	const std::shared_ptr<Pipeline>& pPipeline,
	SyncStatusConstPtr pSyncStatus)
	: m_config(config),
	m_connectionManager(connectionManager),
	m_peerManager(peerManager),
	m_pBlockChain(pBlockChain),
	m_pPipeline(pPipeline),
	m_pSyncStatus(pSyncStatus)
{

}

void MessageProcessor::ProcessMessage(Connection& connection, const RawMessage& rawMessage)
{
	const EMessageType messageType = rawMessage.GetMessageHeader().GetMessageType();

	try
	{
		return ProcessMessageInternal(connection, rawMessage);
	}
	catch (const BadDataException&)
	{
		LOG_ERROR_F("Bad data received in message({}) from ({})", MessageTypes::ToString(messageType), connection);
		connection.BanPeer(EBanReason::Abusive); // TODO: Determine real reason
	}
	catch (const BlockChainException&)
	{
		LOG_ERROR_F("BlockChain exception while processing message({}) from ({})", MessageTypes::ToString(messageType), connection);
		connection.Disconnect();
	}
	catch (const DeserializationException&)
	{
		LOG_ERROR_F("Deserialization exception while processing message({}) from ({})", MessageTypes::ToString(messageType), connection);
		connection.Disconnect();
	}
}

void MessageProcessor::ProcessMessageInternal(Connection& connection, const RawMessage& rawMessage)
{
	const MessageHeader& header = rawMessage.GetMessageHeader();
	EProtocolVersion protocolVersion = connection.GetProtocolVersion();
	ByteBuffer byteBuffer(rawMessage.GetPayload(), protocolVersion);

	switch (header.GetMessageType())
	{
		case Error:
		{
			const ErrorMessage errorMessage = ErrorMessage::Deserialize(byteBuffer);
			LOG_WARNING_F("Error message \"{}\" retrieved from {}", errorMessage.GetErrorMessage(), connection);
			break;
		}
		case BanReasonMsg:
		{
			const BanReasonMessage banReasonMessage = BanReasonMessage::Deserialize(byteBuffer);
			std::string banMessage = BanReason::Format((EBanReason)banReasonMessage.GetBanReason());
			LOG_WARNING_F("BanReason message '{}' retrieved from {}", banMessage, connection);
			break;
		}
		case Ping:
		{
			const PingMessage pingMessage = PingMessage::Deserialize(byteBuffer);
			connection.UpdateTotals(pingMessage.GetTotalDifficulty(), pingMessage.GetHeight());

			auto pTipHeader = m_pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED);
			const PongMessage pongMessage(pTipHeader->GetTotalDifficulty(), pTipHeader->GetHeight());

			connection.SendMsg(pongMessage);
			break;
		}
		case Pong:
		{
			const PongMessage pongMessage = PongMessage::Deserialize(byteBuffer);
			connection.UpdateTotals(pongMessage.GetTotalDifficulty(), pongMessage.GetHeight());
			break;
		}
		case GetPeerAddrs:
		{
			const GetPeerAddressesMessage getPeerAddressesMessage = GetPeerAddressesMessage::Deserialize(byteBuffer);
			const Capabilities capabilities = getPeerAddressesMessage.GetCapabilities();

			const std::vector<PeerPtr> peers = m_peerManager.Read()->GetPeers(capabilities.GetCapability(), P2P::MAX_PEER_ADDRS);
			std::vector<SocketAddress> socketAddresses;
			std::transform(
				peers.cbegin(),
				peers.cend(),
				std::back_inserter(socketAddresses),
				[this](const PeerPtr& peer) { return SocketAddress(peer->GetIPAddress(), this->m_config.GetEnvironment().GetP2PPort()); }
			);

			LOG_TRACE_F("Sending {} addresses to {}.", socketAddresses.size(), connection);
			connection.SendMsg(PeerAddressesMessage{ std::move(socketAddresses) });
			break;
		}
		case PeerAddrs:
		{
			const PeerAddressesMessage peerAddressesMessage = PeerAddressesMessage::Deserialize(byteBuffer);
			const std::vector<SocketAddress>& peerAddresses = peerAddressesMessage.GetPeerAddresses();

			LOG_TRACE_F("Received {} addresses from {}.", peerAddresses.size(), connection);
			m_peerManager.Write()->AddFreshPeers(peerAddresses);
			break;
		}
		case GetHeaders:
		{
			const GetHeadersMessage getHeadersMessage = GetHeadersMessage::Deserialize(byteBuffer);
			const std::vector<Hash>& hashes = getHeadersMessage.GetHashes();

			std::vector<BlockHeaderPtr> blockHeaders = BlockLocator(m_pBlockChain).LocateHeaders(hashes);
			LOG_DEBUG_F("Sending {} headers to {}.", blockHeaders.size(), connection);
                
			connection.SendMsg(HeadersMessage{ std::move(blockHeaders) });
			break;
		}
		case Header:
		{
			auto pBlockHeader = HeaderMessage::Deserialize(byteBuffer).GetHeader();

			if (pBlockHeader->GetTotalDifficulty() > connection.GetTotalDifficulty()) {
				connection.UpdateTotals(pBlockHeader->GetTotalDifficulty(), pBlockHeader->GetHeight());
			}

			if (m_pSyncStatus->GetStatus() != ESyncStatus::NOT_SYNCING) {
				break;
			}

			const EBlockChainStatus status = m_pBlockChain->AddBlockHeader(pBlockHeader);
			if (status == EBlockChainStatus::SUCCESS || status == EBlockChainStatus::ALREADY_EXISTS || status == EBlockChainStatus::ORPHANED) {
				if (!m_pBlockChain->HasBlock(pBlockHeader->GetHeight(), pBlockHeader->GetHash())) {
					LOG_TRACE_F("Valid header {} received from {}. Requesting compact block", *pBlockHeader, connection);
					const GetCompactBlockMessage getCompactBlockMessage(pBlockHeader->GetHash());
					connection.SendMsg(GetCompactBlockMessage{ pBlockHeader->GetHash() });
				}
			} else if (status == EBlockChainStatus::INVALID) {
				connection.BanPeer(EBanReason::BadBlockHeader);
			} else {
				LOG_TRACE_F("Header {} from {} not needed", *pBlockHeader, connection);
			}

			break;
		}
		case Headers:
		{
			const HeadersMessage headersMessage = HeadersMessage::Deserialize(byteBuffer);
			const std::vector<BlockHeaderPtr>& blockHeaders = headersMessage.GetHeaders();

			LOG_DEBUG_F("{} headers received from {}", blockHeaders.size(), connection);

			const EBlockChainStatus status = m_pBlockChain->AddBlockHeaders(blockHeaders);
			if (status == EBlockChainStatus::INVALID) {
				connection.BanPeer(EBanReason::BadBlockHeader);
			}

			LOG_DEBUG_F("Headers message from {} finished processing", connection);
			break;
		}
		case GetBlock:
		{
			const GetBlockMessage getBlockMessage = GetBlockMessage::Deserialize(byteBuffer);
			std::unique_ptr<FullBlock> pBlock = m_pBlockChain->GetBlockByHash(getBlockMessage.GetHash());
			if (pBlock != nullptr) {
				connection.SendMsg(BlockMessage{ std::move(*pBlock) });
			}

			break;
		}
		case Block:
		{
			const BlockMessage blockMessage = BlockMessage::Deserialize(byteBuffer);
			const FullBlock& block = blockMessage.GetBlock();

			LOG_TRACE_F("Block received: {}", block.GetHeight());

			if (m_pSyncStatus->GetStatus() == ESyncStatus::SYNCING_BLOCKS) {
				m_pPipeline->ProcessBlock(connection, block);
			} else {
				const EBlockChainStatus added = m_pBlockChain->AddBlock(block);
				if (added == EBlockChainStatus::SUCCESS) {
					const HeaderMessage headerMessage(block.GetHeader());
					m_connectionManager.BroadcastMessage(headerMessage, connection.GetId());
				} else if (added == EBlockChainStatus::ORPHANED) {
					if (block.GetTotalDifficulty() > m_pBlockChain->GetTotalDifficulty(EChainType::CONFIRMED))
					{
						connection.SendMsg(GetCompactBlockMessage{ block.GetPreviousHash() });
					}
				} else if (added == EBlockChainStatus::INVALID) {
					connection.BanPeer(EBanReason::BadBlock);
				}
			}

			break;
		}
		case GetCompactBlock:
		{
			const GetCompactBlockMessage getCompactBlockMessage = GetCompactBlockMessage::Deserialize(byteBuffer);
			std::unique_ptr<CompactBlock> pCompactBlock = m_pBlockChain->GetCompactBlockByHash(getCompactBlockMessage.GetHash());
			if (pCompactBlock != nullptr)
			{
				connection.SendMsg(CompactBlockMessage{ *pCompactBlock });
			}

			break;
		}
		case CompactBlockMsg:
		{
			const CompactBlockMessage compactBlockMessage = CompactBlockMessage::Deserialize(byteBuffer);
			const CompactBlock& compactBlock = compactBlockMessage.GetCompactBlock();

			const EBlockChainStatus added = m_pBlockChain->AddCompactBlock(compactBlock);
			if (added == EBlockChainStatus::SUCCESS)
			{
				const HeaderMessage headerMessage(compactBlock.GetHeader());
				m_connectionManager.BroadcastMessage(headerMessage, connection.GetId());
				break;
			}
			else if (added == EBlockChainStatus::TRANSACTIONS_MISSING)
			{
				connection.SendMsg(GetBlockMessage{ compactBlock.GetHash() });
				break;
			}
			else if (added == EBlockChainStatus::ORPHANED)
			{
				if (m_pSyncStatus->GetStatus() == ESyncStatus::NOT_SYNCING)
				{
					if (compactBlock.GetTotalDifficulty() > m_pBlockChain->GetTotalDifficulty(EChainType::CONFIRMED))
					{
						connection.SendMsg(GetCompactBlockMessage{ compactBlock.GetPreviousHash() });
					}
				}
			}

			break;
		}
		case StemTransaction:
		{
			if (m_pSyncStatus->GetStatus() == ESyncStatus::NOT_SYNCING) {
				TransactionPtr pTransaction = StemTransactionMessage::Deserialize(byteBuffer).GetTransaction();
				m_pPipeline->ProcessTransaction(connection, pTransaction, EPoolType::STEMPOOL);
			}


			break;
		}
		case TransactionMsg:
		{
			if (m_pSyncStatus->GetStatus() == ESyncStatus::NOT_SYNCING) {
				TransactionPtr pTransaction = TransactionMessage::Deserialize(byteBuffer).GetTransaction();
				m_pPipeline->ProcessTransaction(connection, pTransaction, EPoolType::MEMPOOL);
			}

			break;
		}
		case TxHashSetRequest:
		{
			SendTxHashSet(connection, TxHashSetRequestMessage::Deserialize(byteBuffer));
			break;
		}
		case TxHashSetArchive:
		{
			m_pPipeline->ReceiveTxHashSet(connection, TxHashSetArchiveMessage::Deserialize(byteBuffer));
			break;
		}
		case GetTransactionMsg:
		{
			const GetTransactionMessage getTransactionMessage = GetTransactionMessage::Deserialize(byteBuffer);
			const Hash& kernelHash = getTransactionMessage.GetKernelHash();
			LOG_DEBUG_F("Transaction with kernel {} requested.", kernelHash);

			TransactionPtr pTransaction = m_pBlockChain->GetTransactionByKernelHash(kernelHash);
			if (pTransaction != nullptr) {
				LOG_DEBUG_F("Transaction {} found.", pTransaction);
				connection.SendMsg(TransactionMessage{ pTransaction });
			}

			break;
		}
		case TransactionKernelMsg:
		{
			if (m_pSyncStatus->GetStatus() != ESyncStatus::NOT_SYNCING)
			{
				break;
			}

			Hash kernelHash = TransactionKernelMessage::Deserialize(byteBuffer).GetKernelHash();
			TransactionPtr pTransaction = m_pBlockChain->GetTransactionByKernelHash(kernelHash);
			if (pTransaction == nullptr) {
				connection.SendMsg(GetTransactionMessage{ std::move(kernelHash) });
			}

			break;
		}
		default:
		{
			break;
		}
	}

	return;
}

void MessageProcessor::SendTxHashSet(Connection& connection, const TxHashSetRequestMessage& txHashSetRequestMessage)
{
	const time_t maxTxHashSetRequest = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() - std::chrono::hours(2));
	if (connection.GetPeer()->GetLastTxHashSetRequest() > maxTxHashSetRequest) {
		LOG_WARNING_F("Peer '{}' requested multiple TxHashSet's within 2 hours.", connection.GetIPAddress());
		connection.BanPeer(EBanReason::Abusive);
	}

	LOG_INFO_F("Sending TxHashSet snapshot to {}", connection);
	connection.GetPeer()->UpdateLastTxHashSetRequest();

	auto pHeader = m_pBlockChain->GetBlockHeaderByHash(txHashSetRequestMessage.GetBlockHash());
	if (pHeader == nullptr) {
		return;
	}

	fs::path zipFilePath;
	
	try
	{
		zipFilePath = m_pBlockChain->SnapshotTxHashSet(pHeader);
	}
	catch (std::exception&)
	{
		return;
	}

	// Hack until I can determine why zips aren't deleted.
	FileRemover remover(zipFilePath);

	std::ifstream file(zipFilePath, std::ios::in | std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		FileUtil::RemoveFile(zipFilePath);
		return;
	}
	
	try
	{
		const uint64_t fileSize = FileUtil::GetFileSize(zipFilePath);
		file.seekg(0);
		TxHashSetArchiveMessage archiveMessage(Hash(pHeader->GetHash()), pHeader->GetHeight(), fileSize);
		connection.SendMsg(archiveMessage);

		SocketPtr pSocket = connection.GetSocket();
		pSocket->SetBlocking(false);

		std::vector<unsigned char> buffer(BUFFER_SIZE, 0);
		uint64_t totalBytesRead = 0;
		while (totalBytesRead < fileSize)
		{
			file.read((char*)&buffer[0], BUFFER_SIZE);
			const uint64_t bytesRead = file.gcount();

			const std::vector<unsigned char> bytesToSend(buffer.cbegin(), buffer.cbegin() + bytesRead);
			const bool sent = pSocket->Send(bytesToSend, false);

			if (!sent || !Global::IsRunning()) {
				LOG_ERROR("Transmission ended abruptly");
				file.close();
				FileUtil::RemoveFile(zipFilePath);

				return;
			}

			totalBytesRead += bytesRead;
		}

		pSocket->SetBlocking(true);
	}
	catch (...)
	{
		if (file.is_open())
		{
			file.close();
		}
		FileUtil::RemoveFile(zipFilePath);
		throw;
	}

	file.close();
	FileUtil::RemoveFile(zipFilePath);
}