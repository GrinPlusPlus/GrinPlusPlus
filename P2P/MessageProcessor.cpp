#include "MessageProcessor.h"
#include "MessageSender.h"
#include "Seed/PeerManager.h"
#include "BlockLocator.h"
#include "ConnectionManager.h"

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

#include <P2P/Common.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/StringUtil.h>
#include <Common/Util/FileUtil.h>
#include <BlockChain/BlockChainServer.h>
#include <Infrastructure/Logger.h>
#include <async++.h>
#include <fstream>
#include <filesystem>

static const int BUFFER_SIZE = 256 * 1024;

using namespace MessageTypes;

MessageProcessor::MessageProcessor(const Config& config, ConnectionManager& connectionManager, PeerManager& peerManager, IBlockChainServer& blockChainServer)
	: m_config(config), m_connectionManager(connectionManager), m_peerManager(peerManager), m_blockChainServer(blockChainServer)
{

}

MessageProcessor::EStatus MessageProcessor::ProcessMessage(const uint64_t connectionId, ConnectedPeer& connectedPeer, const RawMessage& rawMessage)
{
	try
	{
		return ProcessMessageInternal(connectionId, connectedPeer, rawMessage);
	}
	catch (const DeserializationException&)
	{
		const EMessageType messageType = rawMessage.GetMessageHeader().GetMessageType();
		const std::string formattedIPAddress = connectedPeer.GetPeer().GetIPAddress().Format();
		LoggerAPI::LogError(StringUtil::Format("MessageProcessor::ProcessMessage - Deserialization exception while processing message(%d) from %s.", messageType, formattedIPAddress.c_str()));

		return EStatus::BAN_PEER;
	}
}

MessageProcessor::EStatus MessageProcessor::ProcessMessageInternal(const uint64_t connectionId, ConnectedPeer& connectedPeer, const RawMessage& rawMessage)
{
	const std::string formattedIPAddress = connectedPeer.GetPeer().GetIPAddress().Format();
	const MessageHeader& header = rawMessage.GetMessageHeader();
	if (header.IsValid(m_config))
	{
		switch (header.GetMessageType())
		{
			case Error:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const ErrorMessage errorMessage = ErrorMessage::Deserialize(byteBuffer);
				LoggerAPI::LogWarning("Error message retrieved from peer(" + formattedIPAddress + "): " + errorMessage.GetErrorMessage());

				return EStatus::BAN_PEER;
			}
			case BanReasonMsg:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const BanReasonMessage banReasonMessage = BanReasonMessage::Deserialize(byteBuffer);
				LoggerAPI::LogWarning("BanReason message retrieved from peer(" + formattedIPAddress + "): " + std::to_string(banReasonMessage.GetBanReason()));

				return EStatus::BAN_PEER;
			}
			case Ping:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const PingMessage pingMessage = PingMessage::Deserialize(byteBuffer);
				connectedPeer.UpdateTotals(pingMessage.GetTotalDifficulty(), pingMessage.GetHeight());

				const PongMessage pongMessage(m_blockChainServer.GetTotalDifficulty(EChainType::CONFIRMED), m_blockChainServer.GetHeight(EChainType::CONFIRMED));

				return MessageSender(m_config).Send(connectedPeer, pongMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
			}
			case Pong:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const PongMessage pongMessage = PongMessage::Deserialize(byteBuffer);
				connectedPeer.UpdateTotals(pongMessage.GetTotalDifficulty(), pongMessage.GetHeight());

				return EStatus::SUCCESS;
			}
			case GetPeerAddrs:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const GetPeerAddressesMessage getPeerAddressesMessage = GetPeerAddressesMessage::Deserialize(byteBuffer);
				const Capabilities capabilities = getPeerAddressesMessage.GetCapabilities();

				const std::vector<Peer> peers = m_peerManager.GetPeers(capabilities.GetCapability(), P2P::MAX_PEER_ADDRS);
				std::vector<SocketAddress> socketAddresses;
				for (const Peer& peer : peers)
				{
					socketAddresses.push_back(peer.GetSocketAddress());
				}

				LoggerAPI::LogTrace(StringUtil::Format("MessageProcessor::ProcessMessageInternal - Sending %llu addresses to %s.", socketAddresses.size(), formattedIPAddress.c_str()));
				const PeerAddressesMessage peerAddressesMessage(std::move(socketAddresses));

				return MessageSender(m_config).Send(connectedPeer, peerAddressesMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
			}
			case PeerAddrs:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const PeerAddressesMessage peerAddressesMessage = PeerAddressesMessage::Deserialize(byteBuffer);
				const std::vector<SocketAddress>& peerAddresses = peerAddressesMessage.GetPeerAddresses();

				LoggerAPI::LogTrace(StringUtil::Format("MessageProcessor::ProcessMessageInternal - Received %llu addresses from %s.", peerAddresses.size(), formattedIPAddress.c_str()));
				m_peerManager.AddPeerAddresses(peerAddresses);

				return EStatus::SUCCESS;
			}
			case GetHeaders:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const GetHeadersMessage getHeadersMessage = GetHeadersMessage::Deserialize(byteBuffer);
				const std::vector<CBigInteger<32>>& hashes = getHeadersMessage.GetHashes();

				std::vector<BlockHeader> blockHeaders = BlockLocator(m_blockChainServer).LocateHeaders(hashes);
				const HeadersMessage headersMessage(std::move(blockHeaders));

				LoggerAPI::LogDebug(StringUtil::Format("MessageProcessor::ProcessMessageInternal - Sending %llu headers to %s.", blockHeaders.size(), formattedIPAddress.c_str()));
				return MessageSender(m_config).Send(connectedPeer, headersMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
			}
			case Header:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const HeaderMessage headerMessage = HeaderMessage::Deserialize(byteBuffer);
				const BlockHeader& blockHeader = headerMessage.GetHeader();

				const EBlockChainStatus status = m_blockChainServer.AddBlockHeader(blockHeader);
				if (status == EBlockChainStatus::SUCCESS || status == EBlockChainStatus::ALREADY_EXISTS || status == EBlockChainStatus::ORPHANED)
				{
					LoggerAPI::LogDebug(StringUtil::Format("MessageProcessor::ProcessMessageInternal - Valid header %s received from %s. Requesting compact block", blockHeader.FormatHash().c_str(), formattedIPAddress.c_str()));

					if (m_blockChainServer.GetBlockByHash(blockHeader.GetHash()) == nullptr)
					{
						const GetCompactBlockMessage getCompactBlockMessage(blockHeader.GetHash());
						return MessageSender(m_config).Send(connectedPeer, getCompactBlockMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
					}
				}
				else
				{
					LoggerAPI::LogTrace(StringUtil::Format("MessageProcessor::ProcessMessageInternal - Header %s from %s not needed.", blockHeader.FormatHash().c_str(), formattedIPAddress.c_str()));
				}

				return (status == EBlockChainStatus::SUCCESS) ? EStatus::SUCCESS : EStatus::UNKNOWN_ERROR;
			}
			case Headers:
			{
				IBlockChainServer& blockChainServer = m_blockChainServer;
				async::spawn([&blockChainServer, rawMessage, formattedIPAddress] {
					ByteBuffer byteBuffer(rawMessage.GetPayload());
					const HeadersMessage headersMessage = HeadersMessage::Deserialize(byteBuffer);
					const std::vector<BlockHeader>& blockHeaders = headersMessage.GetHeaders();

					LoggerAPI::LogDebug(StringUtil::Format("MessageProcessor::ProcessMessageInternal - %lld headers received from %s.", blockHeaders.size(), formattedIPAddress.c_str()));

					blockChainServer.AddBlockHeaders(blockHeaders);
					LoggerAPI::LogTrace(StringUtil::Format("MessageProcessor::ProcessMessageInternal - Headers message from %s finished processing.", formattedIPAddress.c_str()));
				});

				return EStatus::SUCCESS;
			}
			case GetBlock:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const GetBlockMessage getBlockMessage = GetBlockMessage::Deserialize(byteBuffer);
				std::unique_ptr<FullBlock> pBlock = m_blockChainServer.GetBlockByHash(getBlockMessage.GetHash());
				if (pBlock != nullptr)
				{
					BlockMessage blockMessage(std::move(*pBlock));
					return MessageSender(m_config).Send(connectedPeer, blockMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
				}

				return EStatus::UNKNOWN_ERROR;
			}
			case Block:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const BlockMessage blockMessage = BlockMessage::Deserialize(byteBuffer);
				const FullBlock& block = blockMessage.GetBlock();

				LoggerAPI::LogTrace("Block received: " + std::to_string(block.GetBlockHeader().GetHeight()));

				if (m_connectionManager.GetSyncStatus().GetStatus() == ESyncStatus::SYNCING_BLOCKS)
				{
					m_connectionManager.GetPipeline().AddBlockToProcess(connectionId, block);
				}
				else
				{
					const EBlockChainStatus added = m_blockChainServer.AddBlock(block);
					if (added == EBlockChainStatus::SUCCESS)
					{
						const HeaderMessage headerMessage(block.GetBlockHeader());
						m_connectionManager.BroadcastMessage(headerMessage, connectionId);
						return EStatus::SUCCESS;
					}
					else if (added == EBlockChainStatus::ORPHANED)
					{
						if (block.GetBlockHeader().GetTotalDifficulty() > m_blockChainServer.GetTotalDifficulty(EChainType::CONFIRMED))
						{
							const GetCompactBlockMessage getPreviousCompactBlockMessage(block.GetBlockHeader().GetPreviousBlockHash());
							return MessageSender(m_config).Send(connectedPeer, getPreviousCompactBlockMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
						}
					}
					else if (added == EBlockChainStatus::INVALID)
					{
						return EStatus::BAN_PEER;
					}
				}

				return EStatus::SUCCESS;
			}
			case GetCompactBlock:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const GetCompactBlockMessage getCompactBlockMessage = GetCompactBlockMessage::Deserialize(byteBuffer);
				std::unique_ptr<CompactBlock> pCompactBlock = m_blockChainServer.GetCompactBlockByHash(getCompactBlockMessage.GetHash());
				if (pCompactBlock != nullptr)
				{
					const CompactBlockMessage compactBlockMessage(*pCompactBlock);
					return MessageSender(m_config).Send(connectedPeer, compactBlockMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
				}

				return EStatus::SUCCESS;
			}
			case CompactBlockMsg:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const CompactBlockMessage compactBlockMessage = CompactBlockMessage::Deserialize(byteBuffer);
				const CompactBlock& compactBlock = compactBlockMessage.GetCompactBlock();

				const EBlockChainStatus added = m_blockChainServer.AddCompactBlock(compactBlock);
				if (added == EBlockChainStatus::SUCCESS)
				{
					const HeaderMessage headerMessage(compactBlock.GetBlockHeader());
					m_connectionManager.BroadcastMessage(headerMessage, connectionId);
					return EStatus::SUCCESS;
				}
				else if (added == EBlockChainStatus::TRANSACTIONS_MISSING)
				{
					const GetBlockMessage getBlockMessage(compactBlock.GetHash());
					return MessageSender(m_config).Send(connectedPeer, getBlockMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
				}
				else if (added == EBlockChainStatus::ORPHANED)
				{
					if (m_connectionManager.GetSyncStatus().GetStatus() == ESyncStatus::NOT_SYNCING)
					{
						if (compactBlock.GetBlockHeader().GetTotalDifficulty() > m_blockChainServer.GetTotalDifficulty(EChainType::CONFIRMED))
						{
							const GetCompactBlockMessage getPreviousCompactBlockMessage(compactBlock.GetBlockHeader().GetPreviousBlockHash());
							return MessageSender(m_config).Send(connectedPeer, getPreviousCompactBlockMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
						}
					}
				}

				return EStatus::UNKNOWN_ERROR;
			}
			case StemTransaction:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const StemTransactionMessage transactionMessage = StemTransactionMessage::Deserialize(byteBuffer);
				const Transaction& transaction = transactionMessage.GetTransaction();

				if (m_connectionManager.GetSyncStatus().GetStatus() == ESyncStatus::NOT_SYNCING)
				{
					return m_connectionManager.GetPipeline().AddTransactionToProcess(connectionId, transaction, EPoolType::STEMPOOL) ? EStatus::SUCCESS : EStatus::UNKNOWN_ERROR;
				}

				return EStatus::UNKNOWN_ERROR;
			}
			case TransactionMsg:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const TransactionMessage transactionMessage = TransactionMessage::Deserialize(byteBuffer);
				const Transaction& transaction = transactionMessage.GetTransaction();

				if (m_connectionManager.GetSyncStatus().GetStatus() == ESyncStatus::NOT_SYNCING)
				{
					return m_connectionManager.GetPipeline().AddTransactionToProcess(connectionId, transaction, EPoolType::MEMPOOL) ? EStatus::SUCCESS : EStatus::UNKNOWN_ERROR;
				}

				return EStatus::UNKNOWN_ERROR;
			}
			case TxHashSetRequest:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const TxHashSetRequestMessage txHashSetRequestMessage = TxHashSetRequestMessage::Deserialize(byteBuffer);

				return SendTxHashSet(connectionId, connectedPeer, txHashSetRequestMessage);
			}
			case TxHashSetArchive:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const TxHashSetArchiveMessage txHashSetArchiveMessage = TxHashSetArchiveMessage::Deserialize(byteBuffer);

				return ReceiveTxHashSet(connectionId, connectedPeer, txHashSetArchiveMessage);
			}
			default:
			{
				break;
			}
		}
	}

	return EStatus::SUCCESS;
}

MessageProcessor::EStatus MessageProcessor::SendTxHashSet(const uint64_t connectionId, ConnectedPeer& connectedPeer, const TxHashSetRequestMessage& txHashSetRequestMessage)
{
	LoggerAPI::LogInfo(StringUtil::Format("MessageProcessor::SendTxHashSet - Sending TxHashSet snapshot to %s.", connectedPeer.GetPeer().GetIPAddress().Format().c_str()));

	std::unique_ptr<BlockHeader> pHeader = m_blockChainServer.GetBlockHeaderByHash(txHashSetRequestMessage.GetBlockHash());
	if (pHeader == nullptr)
	{
		return EStatus::UNKNOWN_ERROR;
	}

	const std::string zipFilePath = m_blockChainServer.SnapshotTxHashSet(*pHeader);
	if (zipFilePath.empty())
	{
		return EStatus::UNKNOWN_ERROR;
	}

	std::ifstream file(zipFilePath, std::ios::in | std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		return EStatus::UNKNOWN_ERROR;
	}

	const uint64_t fileSize = file.tellg();
	file.seekg(0);
	TxHashSetArchiveMessage archiveMessage(Hash(pHeader->GetHash()), pHeader->GetHeight(), fileSize);
	MessageSender(m_config).Send(connectedPeer, archiveMessage);

	unsigned long nonblocking = 0;
	ioctlsocket(connectedPeer.GetSocket(), FIONBIO, &nonblocking);
	std::vector<unsigned char> buffer(BUFFER_SIZE, 0);
	uint64_t totalBytesRead = 0;
	while (totalBytesRead < fileSize)
	{
		file.read((char*)&buffer[0], BUFFER_SIZE);
		const uint64_t bytesRead = file.gcount();

		uint64_t tempBytesSent = send(connectedPeer.GetSocket(), (char*)&buffer[0], bytesRead, 0);
		uint64_t intervalBytesSent = tempBytesSent;
		while (tempBytesSent > 0 && intervalBytesSent < bytesRead)
		{
			tempBytesSent = send(connectedPeer.GetSocket(), (char*)&buffer[intervalBytesSent], bytesRead - intervalBytesSent, 0);
			intervalBytesSent += tempBytesSent;
		}

		if (intervalBytesSent < bytesRead || m_connectionManager.IsTerminating())
		{
			LoggerAPI::LogError("MessageProcessor::SendTxHashSet - Transmission ended abruptly.");
			file.close();
			FileUtil::RemoveFile(zipFilePath);

			return EStatus::BAN_PEER;
		}

		totalBytesRead += bytesRead;
	}

	file.close();
	FileUtil::RemoveFile(zipFilePath);

	return EStatus::SUCCESS;
}

MessageProcessor::EStatus MessageProcessor::ReceiveTxHashSet(const uint64_t connectionId, ConnectedPeer& connectedPeer, const TxHashSetArchiveMessage& txHashSetArchiveMessage)
{
	LoggerAPI::LogInfo(StringUtil::Format("MessageProcessor::ReceiveTxHashSet - Downloading TxHashSet from %s.", connectedPeer.GetPeer().GetIPAddress().Format().c_str()));

	SyncStatus& syncStatus = m_connectionManager.GetSyncStatus();
	syncStatus.UpdateDownloaded(0);
	syncStatus.UpdateDownloadSize(txHashSetArchiveMessage.GetZippedSize());

	const DWORD timeout = 10 * 1000;
	setsockopt(connectedPeer.GetSocket(), SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	const int socketRcvBuff = BUFFER_SIZE;
	setsockopt(connectedPeer.GetSocket(), SOL_SOCKET, SO_RCVBUF, (const char*)&socketRcvBuff, sizeof(int));

	const std::string hashStr = HexUtil::ConvertHash(txHashSetArchiveMessage.GetBlockHash());
	const std::string txHashSetPath = m_config.GetTxHashSetDirectory() + StringUtil::Format("txhashset_%s.zip", hashStr.c_str());
	std::ofstream fout;
	fout.open(txHashSetPath, std::ios::binary | std::ios::out | std::ios::trunc);

	size_t bytesReceived = 0;
	std::vector<unsigned char> buffer(BUFFER_SIZE, 0);
	while (bytesReceived < txHashSetArchiveMessage.GetZippedSize())
	{
		const int expectedBytes = std::min((int)(txHashSetArchiveMessage.GetZippedSize() - bytesReceived), BUFFER_SIZE);
		const int newBytesReceived = recv(connectedPeer.GetSocket(), (char*)&buffer[0], expectedBytes, 0);
		if (newBytesReceived <= 0 || m_connectionManager.IsTerminating())
		{
			syncStatus.UpdateStatus(ESyncStatus::TXHASHSET_SYNC_FAILED);

			LoggerAPI::LogError("MessageProcessor::ReceiveTxHashSet - Transmission ended abruptly.");
			fout.close();
			FileUtil::RemoveFile(txHashSetPath);

			return EStatus::BAN_PEER;
		}

		fout.write((char*)&buffer[0], newBytesReceived);
		bytesReceived += newBytesReceived;

		syncStatus.UpdateDownloaded(bytesReceived);
	}

	fout.close();

	LoggerAPI::LogInfo("MessageProcessor::ReceiveTxHashSet - Downloading successful.");
	syncStatus.UpdateStatus(ESyncStatus::PROCESSING_TXHASHSET);

	// TODO: Process asynchronously in pipeline.
	const EBlockChainStatus processStatus = m_blockChainServer.ProcessTransactionHashSet(txHashSetArchiveMessage.GetBlockHash(), txHashSetPath);
	if (processStatus == EBlockChainStatus::INVALID)
	{
		syncStatus.UpdateStatus(ESyncStatus::TXHASHSET_SYNC_FAILED);
		return EStatus::BAN_PEER;
	}

	return EStatus::SUCCESS;
}