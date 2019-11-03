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
#include "Messages/GetTransactionMessage.h"
#include "Messages/TransactionKernelMessage.h"

#include <Core/Exceptions/BadDataException.h>
#include <P2P/Common.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/StringUtil.h>
#include <Common/Util/FileUtil.h>
#include <BlockChain/BlockChainServer.h>
#include <Infrastructure/Logger.h>
#include <thread>
#include <fstream>

static const int BUFFER_SIZE = 256 * 1024;

using namespace MessageTypes;

MessageProcessor::MessageProcessor(const Config& config, ConnectionManager& connectionManager, PeerManager& peerManager, IBlockChainServerPtr pBlockChainServer)
	: m_config(config), m_connectionManager(connectionManager), m_peerManager(peerManager), m_pBlockChainServer(pBlockChainServer)
{

}

MessageProcessor::EStatus MessageProcessor::ProcessMessage(const uint64_t connectionId, Socket& socket, ConnectedPeer& connectedPeer, const RawMessage& rawMessage)
{
	try
	{
		return ProcessMessageInternal(connectionId, socket, connectedPeer, rawMessage);
	}
	catch (const BadDataException& e)
	{
		const EMessageType messageType = rawMessage.GetMessageHeader().GetMessageType();
		LOG_ERROR_F("Bad data received in message(%d) from %s", messageType, connectedPeer.GetPeer());

		return EStatus::BAN_PEER;
	}
	catch (const DeserializationException&)
	{
		const EMessageType messageType = rawMessage.GetMessageHeader().GetMessageType();
		LOG_ERROR_F("Deserialization exception while processing message(%d) from %s", messageType, connectedPeer.GetPeer());

		return EStatus::BAN_PEER;
	}
}

MessageProcessor::EStatus MessageProcessor::ProcessMessageInternal(const uint64_t connectionId, Socket& socket, ConnectedPeer& connectedPeer, const RawMessage& rawMessage)
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
				LOG_WARNING("Error message retrieved from peer(" + formattedIPAddress + "): " + errorMessage.GetErrorMessage());

				return EStatus::BAN_PEER;
			}
			case BanReasonMsg:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const BanReasonMessage banReasonMessage = BanReasonMessage::Deserialize(byteBuffer);
				LOG_WARNING("BanReason message retrieved from peer(" + formattedIPAddress + "): " + BanReason::Format((EBanReason)banReasonMessage.GetBanReason()));

				return EStatus::BAN_PEER;
			}
			case Ping:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const PingMessage pingMessage = PingMessage::Deserialize(byteBuffer);
				connectedPeer.UpdateTotals(pingMessage.GetTotalDifficulty(), pingMessage.GetHeight());

				const PongMessage pongMessage(m_pBlockChainServer->GetTotalDifficulty(EChainType::CONFIRMED), m_pBlockChainServer->GetHeight(EChainType::CONFIRMED));

				return MessageSender(m_config).Send(socket, pongMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
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

				LOG_TRACE_F("Sending %llu addresses to %s.", socketAddresses.size(), formattedIPAddress);
				const PeerAddressesMessage peerAddressesMessage(std::move(socketAddresses));

				return MessageSender(m_config).Send(socket, peerAddressesMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
			}
			case PeerAddrs:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const PeerAddressesMessage peerAddressesMessage = PeerAddressesMessage::Deserialize(byteBuffer);
				const std::vector<SocketAddress>& peerAddresses = peerAddressesMessage.GetPeerAddresses();

				LOG_TRACE_F("Received %llu addresses from %s.", peerAddresses.size(), formattedIPAddress);
				m_peerManager.AddFreshPeers(peerAddresses);

				return EStatus::SUCCESS;
			}
			case GetHeaders:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const GetHeadersMessage getHeadersMessage = GetHeadersMessage::Deserialize(byteBuffer);
				const std::vector<CBigInteger<32>>& hashes = getHeadersMessage.GetHashes();

				std::vector<BlockHeader> blockHeaders = BlockLocator(m_pBlockChainServer).LocateHeaders(hashes);
				LOG_DEBUG_F("Sending %llu headers to %s.", blockHeaders.size(), formattedIPAddress);
                
				const HeadersMessage headersMessage(std::move(blockHeaders));
				return MessageSender(m_config).Send(socket, headersMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
			}
			case Header:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const HeaderMessage headerMessage = HeaderMessage::Deserialize(byteBuffer);
				const BlockHeader& blockHeader = headerMessage.GetHeader();

				if (blockHeader.GetTotalDifficulty() > connectedPeer.GetTotalDifficulty())
				{
					connectedPeer.UpdateTotals(blockHeader.GetTotalDifficulty(), blockHeader.GetHeight());
				}

				const EBlockChainStatus status = m_pBlockChainServer->AddBlockHeader(blockHeader);
				if (status == EBlockChainStatus::SUCCESS || status == EBlockChainStatus::ALREADY_EXISTS || status == EBlockChainStatus::ORPHANED)
				{
					LOG_DEBUG_F("Valid header %s received from %s. Requesting compact block", blockHeader, formattedIPAddress);

					if (m_pBlockChainServer->GetBlockByHash(blockHeader.GetHash()) == nullptr)
					{
						const GetCompactBlockMessage getCompactBlockMessage(blockHeader.GetHash());
						return MessageSender(m_config).Send(socket, getCompactBlockMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
					}
				}
				else if (status == EBlockChainStatus::INVALID)
				{
					return EStatus::BAN_PEER;
				}
				else
				{
					LOG_TRACE_F("Header %s from %s not needed", blockHeader, formattedIPAddress);
				}

				return (status == EBlockChainStatus::SUCCESS) ? EStatus::SUCCESS : EStatus::UNKNOWN_ERROR;
			}
			case Headers:
			{
				IBlockChainServerPtr pBlockChainServer = m_pBlockChainServer;
				std::thread processHeaders([pBlockChainServer, rawMessage, formattedIPAddress] {
					try
					{
						ByteBuffer byteBuffer(rawMessage.GetPayload());
						const HeadersMessage headersMessage = HeadersMessage::Deserialize(byteBuffer);
						const std::vector<BlockHeader>& blockHeaders = headersMessage.GetHeaders();

						LOG_DEBUG_F("%lld headers received from %s", blockHeaders.size(), formattedIPAddress);

						const EBlockChainStatus status = pBlockChainServer->AddBlockHeaders(blockHeaders); // TODO: Handle failures
						LOG_DEBUG_F("Headers message from %s finished processing", formattedIPAddress);
					}
					catch (std::exception& e)
					{
						LOG_ERROR_F("Failed to process headers with exception: %s", e.what());
					}
				});
				processHeaders.detach();

				return EStatus::SUCCESS;
			}
			case GetBlock:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const GetBlockMessage getBlockMessage = GetBlockMessage::Deserialize(byteBuffer);
				std::unique_ptr<FullBlock> pBlock = m_pBlockChainServer->GetBlockByHash(getBlockMessage.GetHash());
				if (pBlock != nullptr)
				{
					BlockMessage blockMessage(std::move(*pBlock));
					return MessageSender(m_config).Send(socket, blockMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
				}

				return EStatus::RESOURCE_NOT_FOUND;
			}
			case Block:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const BlockMessage blockMessage = BlockMessage::Deserialize(byteBuffer);
				const FullBlock& block = blockMessage.GetBlock();

				LOG_TRACE_F("Block received: %llu", block.GetBlockHeader().GetHeight());

				if (m_connectionManager.GetSyncStatus().GetStatus() == ESyncStatus::SYNCING_BLOCKS)
				{
					m_connectionManager.GetPipeline().GetBlockPipe().AddBlockToProcess(connectionId, block);
				}
				else
				{
					const EBlockChainStatus added = m_pBlockChainServer->AddBlock(block);
					if (added == EBlockChainStatus::SUCCESS)
					{
						const HeaderMessage headerMessage(block.GetBlockHeader());
						m_connectionManager.BroadcastMessage(headerMessage, connectionId);
						return EStatus::SUCCESS;
					}
					else if (added == EBlockChainStatus::ORPHANED)
					{
						if (block.GetBlockHeader().GetTotalDifficulty() > m_pBlockChainServer->GetTotalDifficulty(EChainType::CONFIRMED))
						{
							const GetCompactBlockMessage getPreviousCompactBlockMessage(block.GetBlockHeader().GetPreviousBlockHash());
							return MessageSender(m_config).Send(socket, getPreviousCompactBlockMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
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
				std::unique_ptr<CompactBlock> pCompactBlock = m_pBlockChainServer->GetCompactBlockByHash(getCompactBlockMessage.GetHash());
				if (pCompactBlock != nullptr)
				{
					const CompactBlockMessage compactBlockMessage(*pCompactBlock);
					return MessageSender(m_config).Send(socket, compactBlockMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
				}

				return EStatus::RESOURCE_NOT_FOUND;
			}
			case CompactBlockMsg:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const CompactBlockMessage compactBlockMessage = CompactBlockMessage::Deserialize(byteBuffer);
				const CompactBlock& compactBlock = compactBlockMessage.GetCompactBlock();

				const EBlockChainStatus added = m_pBlockChainServer->AddCompactBlock(compactBlock);
				if (added == EBlockChainStatus::SUCCESS)
				{
					const HeaderMessage headerMessage(compactBlock.GetBlockHeader());
					m_connectionManager.BroadcastMessage(headerMessage, connectionId);
					return EStatus::SUCCESS;
				}
				else if (added == EBlockChainStatus::TRANSACTIONS_MISSING)
				{
					const GetBlockMessage getBlockMessage(compactBlock.GetHash());
					return MessageSender(m_config).Send(socket, getBlockMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
				}
				else if (added == EBlockChainStatus::ORPHANED)
				{
					if (m_connectionManager.GetSyncStatus().GetStatus() == ESyncStatus::NOT_SYNCING)
					{
						if (compactBlock.GetBlockHeader().GetTotalDifficulty() > m_pBlockChainServer->GetTotalDifficulty(EChainType::CONFIRMED))
						{
							const GetCompactBlockMessage getPreviousCompactBlockMessage(compactBlock.GetBlockHeader().GetPreviousBlockHash());
							return MessageSender(m_config).Send(socket, getPreviousCompactBlockMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
						}
					}
				}

				return EStatus::UNKNOWN_ERROR;
			}
			case StemTransaction:
			{
				if (m_connectionManager.GetSyncStatus().GetStatus() != ESyncStatus::NOT_SYNCING)
				{
					return EStatus::SYNCING;
				}

				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const StemTransactionMessage transactionMessage = StemTransactionMessage::Deserialize(byteBuffer);
				const Transaction& transaction = transactionMessage.GetTransaction();

				return m_connectionManager.GetPipeline().GetTransactionPipe().AddTransactionToProcess(connectionId, transaction, EPoolType::STEMPOOL) ? EStatus::SUCCESS : EStatus::UNKNOWN_ERROR;
			}
			case TransactionMsg:
			{
				if (m_connectionManager.GetSyncStatus().GetStatus() != ESyncStatus::NOT_SYNCING)
				{
					return EStatus::SYNCING;
				}

				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const TransactionMessage transactionMessage = TransactionMessage::Deserialize(byteBuffer);
				const Transaction& transaction = transactionMessage.GetTransaction();

				return m_connectionManager.GetPipeline().GetTransactionPipe().AddTransactionToProcess(connectionId, transaction, EPoolType::MEMPOOL) ? EStatus::SUCCESS : EStatus::UNKNOWN_ERROR;
			}
			case TxHashSetRequest:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const TxHashSetRequestMessage txHashSetRequestMessage = TxHashSetRequestMessage::Deserialize(byteBuffer);

				return SendTxHashSet(connectedPeer, socket, txHashSetRequestMessage);
			}
			case TxHashSetArchive:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const TxHashSetArchiveMessage txHashSetArchiveMessage = TxHashSetArchiveMessage::Deserialize(byteBuffer);

				return m_connectionManager.GetPipeline().GetTxHashSetPipe().ReceiveTxHashSet(connectionId, socket, txHashSetArchiveMessage) ? EStatus::SUCCESS : EStatus::BAN_PEER;
			}
			case GetTransactionMsg:
			{
				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const GetTransactionMessage getTransactionMessage = GetTransactionMessage::Deserialize(byteBuffer);
				const Hash& kernelHash = getTransactionMessage.GetKernelHash();

				std::unique_ptr<Transaction> pTransaction = m_pBlockChainServer->GetTransactionByKernelHash(kernelHash);
				if (pTransaction != nullptr)
				{
					const TransactionMessage transactionMessage(*pTransaction);
					return MessageSender(m_config).Send(socket, transactionMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
				}

				return EStatus::RESOURCE_NOT_FOUND;
			}
			case TransactionKernelMsg:
			{
				if (m_connectionManager.GetSyncStatus().GetStatus() != ESyncStatus::NOT_SYNCING)
				{
					return EStatus::SYNCING;
				}

				ByteBuffer byteBuffer(rawMessage.GetPayload());
				const TransactionKernelMessage transactionKernelMessage = TransactionKernelMessage::Deserialize(byteBuffer);
				const Hash& kernelHash = transactionKernelMessage.GetKernelHash();

				// TODO: Also check pipeline
				std::unique_ptr<Transaction> pTransaction = m_pBlockChainServer->GetTransactionByKernelHash(kernelHash);
				if (pTransaction == nullptr)
				{
					const GetTransactionMessage getTransactionMessage(kernelHash);
					return MessageSender(m_config).Send(socket, getTransactionMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
				}

				return EStatus::RESOURCE_NOT_FOUND;
			}
			default:
			{
				break;
			}
		}
	}

	return EStatus::UNKNOWN_MESSAGE;
}

MessageProcessor::EStatus MessageProcessor::SendTxHashSet(ConnectedPeer& peer, Socket& socket, const TxHashSetRequestMessage& txHashSetRequestMessage)
{
	const time_t maxTxHashSetRequest = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() - std::chrono::hours(2));
	if (peer.GetPeer().GetLastTxHashSetRequest() > maxTxHashSetRequest)
	{
		LOG_WARNING_F("Peer (%s) requested multiple TxHashSet's within 2 hours.", socket.GetIPAddress());
		return EStatus::BAN_PEER;
	}

	LOG_INFO_F("Sending TxHashSet snapshot to %s", socket.GetIPAddress());
	peer.GetPeer().UpdateLastTxHashSetRequest();

	std::unique_ptr<BlockHeader> pHeader = m_pBlockChainServer->GetBlockHeaderByHash(txHashSetRequestMessage.GetBlockHash());
	if (pHeader == nullptr)
	{
		return EStatus::UNKNOWN_ERROR;
	}

	const std::string zipFilePath = m_pBlockChainServer->SnapshotTxHashSet(*pHeader);
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
	MessageSender(m_config).Send(socket, archiveMessage);

	socket.SetBlocking(false);

	std::vector<unsigned char> buffer(BUFFER_SIZE, 0);
	uint64_t totalBytesRead = 0;
	while (totalBytesRead < fileSize)
	{
		file.read((char*)&buffer[0], BUFFER_SIZE);
		const uint64_t bytesRead = file.gcount();

		const std::vector<unsigned char> bytesToSend(buffer.cbegin(), buffer.cbegin() + bytesRead);
		const bool sent = socket.Send(bytesToSend, false);

		if (!sent || m_connectionManager.IsTerminating())
		{
			LOG_ERROR("Transmission ended abruptly");
			file.close();
			FileUtil::RemoveFile(zipFilePath);

			return EStatus::BAN_PEER;
		}

		totalBytesRead += bytesRead;
	}

	socket.SetBlocking(true);

	file.close();
	FileUtil::RemoveFile(zipFilePath);

	return EStatus::SUCCESS;
}