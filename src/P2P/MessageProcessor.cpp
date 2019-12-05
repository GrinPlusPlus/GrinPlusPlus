#include "MessageProcessor.h"
#include "MessageSender.h"
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

#include <Core/Exceptions/BadDataException.h>
#include <Core/Exceptions/BlockChainException.h>
#include <P2P/Common.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/StringUtil.h>
#include <Common/Util/FileUtil.h>
#include <BlockChain/BlockChainServer.h>
#include <Infrastructure/ShutdownManager.h>
#include <Infrastructure/Logger.h>
#include <thread>
#include <fstream>

static const int BUFFER_SIZE = 256 * 1024;

using namespace MessageTypes;

MessageProcessor::MessageProcessor(
	const Config& config,
	ConnectionManager& connectionManager,
	Locked<PeerManager> peerManager,
	IBlockChainServerPtr pBlockChainServer,
	Pipeline& pipeline,
	SyncStatusConstPtr pSyncStatus)
	: m_config(config),
	m_connectionManager(connectionManager),
	m_peerManager(peerManager),
	m_pBlockChainServer(pBlockChainServer),
	m_pipeline(pipeline),
	m_pSyncStatus(pSyncStatus)
{

}

MessageProcessor::EStatus MessageProcessor::ProcessMessage(
	const uint64_t connectionId,
	Socket& socket,
	ConnectedPeer& connectedPeer,
	const RawMessage& rawMessage)
{
	const EMessageType messageType = rawMessage.GetMessageHeader().GetMessageType();

	try
	{
		return ProcessMessageInternal(connectionId, socket, connectedPeer, rawMessage);
	}
	catch (const BadDataException&)
	{
		LOG_ERROR_F("Bad data received in message({}) from ({})", MessageTypes::ToString(messageType), connectedPeer.GetPeer());

		return EStatus::BAN_PEER;
	}
	catch (const BlockChainException&)
	{
		LOG_ERROR_F("BlockChain exception while processing message({}) from ({})", MessageTypes::ToString(messageType), connectedPeer.GetPeer());

		return EStatus::BAN_PEER;
	}
	catch (const DeserializationException&)
	{
		LOG_ERROR_F("Deserialization exception while processing message({}) from ({})", MessageTypes::ToString(messageType), connectedPeer.GetPeer());

		return EStatus::BAN_PEER;
	}
}

MessageProcessor::EStatus MessageProcessor::ProcessMessageInternal(
	const uint64_t connectionId,
	Socket& socket,
	ConnectedPeer& connectedPeer,
	const RawMessage& rawMessage)
{
	const std::string formattedIPAddress = connectedPeer.GetPeer().GetIPAddress().Format();
	const MessageHeader& header = rawMessage.GetMessageHeader();
	ByteBuffer byteBuffer(rawMessage.GetPayload());

	if (header.IsValid(m_config))
	{
		switch (header.GetMessageType())
		{
			case Error:
			{
				const ErrorMessage errorMessage = ErrorMessage::Deserialize(byteBuffer);
				LOG_WARNING("Error message retrieved from peer(" + formattedIPAddress + "): " + errorMessage.GetErrorMessage());

				return EStatus::BAN_PEER;
			}
			case BanReasonMsg:
			{
				const BanReasonMessage banReasonMessage = BanReasonMessage::Deserialize(byteBuffer);
				LOG_WARNING("BanReason message retrieved from peer(" + formattedIPAddress + "): " + BanReason::Format((EBanReason)banReasonMessage.GetBanReason()));

				return EStatus::BAN_PEER;
			}
			case Ping:
			{
				const PingMessage pingMessage = PingMessage::Deserialize(byteBuffer);
				connectedPeer.UpdateTotals(pingMessage.GetTotalDifficulty(), pingMessage.GetHeight());

				auto pTipHeader = m_pBlockChainServer->GetTipBlockHeader(EChainType::CONFIRMED);
				const PongMessage pongMessage(pTipHeader->GetTotalDifficulty(), pTipHeader->GetHeight());

				return MessageSender(m_config).Send(socket, pongMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
			}
			case Pong:
			{
				const PongMessage pongMessage = PongMessage::Deserialize(byteBuffer);
				connectedPeer.UpdateTotals(pongMessage.GetTotalDifficulty(), pongMessage.GetHeight());

				return EStatus::SUCCESS;
			}
			case GetPeerAddrs:
			{
				const GetPeerAddressesMessage getPeerAddressesMessage = GetPeerAddressesMessage::Deserialize(byteBuffer);
				const Capabilities capabilities = getPeerAddressesMessage.GetCapabilities();

				const std::vector<Peer> peers = m_peerManager.Read()->GetPeers(capabilities.GetCapability(), P2P::MAX_PEER_ADDRS);
				std::vector<SocketAddress> socketAddresses;
				std::transform(
					peers.cbegin(),
					peers.cend(),
					std::back_inserter(socketAddresses),
					[](const Peer& peer) { return peer.GetSocketAddress(); }
				);

				LOG_TRACE_F("Sending {} addresses to {}.", socketAddresses.size(), formattedIPAddress);
				const PeerAddressesMessage peerAddressesMessage(std::move(socketAddresses));

				return MessageSender(m_config).Send(socket, peerAddressesMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
			}
			case PeerAddrs:
			{
				const PeerAddressesMessage peerAddressesMessage = PeerAddressesMessage::Deserialize(byteBuffer);
				const std::vector<SocketAddress>& peerAddresses = peerAddressesMessage.GetPeerAddresses();

				LOG_TRACE_F("Received {} addresses from {}.", peerAddresses.size(), formattedIPAddress);
				m_peerManager.Write()->AddFreshPeers(peerAddresses);

				return EStatus::SUCCESS;
			}
			case GetHeaders:
			{
				const GetHeadersMessage getHeadersMessage = GetHeadersMessage::Deserialize(byteBuffer);
				const std::vector<Hash>& hashes = getHeadersMessage.GetHashes();

				std::vector<BlockHeaderPtr> blockHeaders = BlockLocator(m_pBlockChainServer).LocateHeaders(hashes);
				LOG_DEBUG_F("Sending {} headers to {}.", blockHeaders.size(), formattedIPAddress);
                
				const HeadersMessage headersMessage(std::move(blockHeaders));
				return MessageSender(m_config).Send(socket, headersMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
			}
			case Header:
			{
				const HeaderMessage headerMessage = HeaderMessage::Deserialize(byteBuffer);
				auto pBlockHeader = headerMessage.GetHeader();

				if (pBlockHeader->GetTotalDifficulty() > connectedPeer.GetTotalDifficulty())
				{
					connectedPeer.UpdateTotals(pBlockHeader->GetTotalDifficulty(), pBlockHeader->GetHeight());
				}

				if (m_pSyncStatus->GetStatus() != ESyncStatus::NOT_SYNCING)
				{
					return EStatus::SUCCESS;
				}

				const EBlockChainStatus status = m_pBlockChainServer->AddBlockHeader(pBlockHeader);
				if (status == EBlockChainStatus::SUCCESS || status == EBlockChainStatus::ALREADY_EXISTS || status == EBlockChainStatus::ORPHANED)
				{
					LOG_DEBUG_F("Valid header {} received from {}. Requesting compact block", *pBlockHeader, formattedIPAddress);

					if (!m_pBlockChainServer->HasBlock(pBlockHeader->GetHeight(), pBlockHeader->GetHash()))
					{
						const GetCompactBlockMessage getCompactBlockMessage(pBlockHeader->GetHash());
						return MessageSender(m_config).Send(socket, getCompactBlockMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
					}
				}
				else if (status == EBlockChainStatus::INVALID)
				{
					return EStatus::BAN_PEER;
				}
				else
				{
					LOG_TRACE_F("Header {} from {} not needed", *pBlockHeader, formattedIPAddress);
				}

				return (status == EBlockChainStatus::SUCCESS) ? EStatus::SUCCESS : EStatus::UNKNOWN_ERROR;
			}
			case Headers:
			{
				IBlockChainServerPtr pBlockChainServer = m_pBlockChainServer;

				const HeadersMessage headersMessage = HeadersMessage::Deserialize(byteBuffer);
				const std::vector<BlockHeaderPtr>& blockHeaders = headersMessage.GetHeaders();

				LOG_DEBUG_F("{} headers received from {}", blockHeaders.size(), formattedIPAddress);

				const EBlockChainStatus status = pBlockChainServer->AddBlockHeaders(blockHeaders);
				LOG_DEBUG_F("Headers message from {} finished processing", formattedIPAddress);

				return status == EBlockChainStatus::INVALID ? EStatus::BAN_PEER : EStatus::SUCCESS;
			}
			case GetBlock:
			{
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
				const BlockMessage blockMessage = BlockMessage::Deserialize(byteBuffer);
				const FullBlock& block = blockMessage.GetBlock();

				LOG_TRACE_F("Block received: {}", block.GetHeight());

				if (m_pSyncStatus->GetStatus() == ESyncStatus::SYNCING_BLOCKS)
				{
					m_pipeline.GetBlockPipe()->AddBlockToProcess(connectionId, block);
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
						if (block.GetTotalDifficulty() > m_pBlockChainServer->GetTotalDifficulty(EChainType::CONFIRMED))
						{
							const GetCompactBlockMessage getPreviousCompactBlockMessage(block.GetPreviousHash());
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
					if (m_pSyncStatus->GetStatus() == ESyncStatus::NOT_SYNCING)
					{
						if (compactBlock.GetTotalDifficulty() > m_pBlockChainServer->GetTotalDifficulty(EChainType::CONFIRMED))
						{
							const GetCompactBlockMessage getPreviousCompactBlockMessage(compactBlock.GetPreviousHash());
							return MessageSender(m_config).Send(socket, getPreviousCompactBlockMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
						}
					}
				}

				return EStatus::UNKNOWN_ERROR;
			}
			case StemTransaction:
			{
				if (m_pSyncStatus->GetStatus() != ESyncStatus::NOT_SYNCING)
				{
					return EStatus::SYNCING;
				}

				const StemTransactionMessage transactionMessage = StemTransactionMessage::Deserialize(byteBuffer);
				TransactionPtr pTransaction = transactionMessage.GetTransaction();

				m_pipeline.GetTransactionPipe()->AddTransactionToProcess(connectionId, pTransaction, EPoolType::STEMPOOL);

				return EStatus::SUCCESS;
			}
			case TransactionMsg:
			{
				if (m_pSyncStatus->GetStatus() != ESyncStatus::NOT_SYNCING)
				{
					return EStatus::SYNCING;
				}

				const TransactionMessage transactionMessage = TransactionMessage::Deserialize(byteBuffer);
				TransactionPtr pTransaction = transactionMessage.GetTransaction();

				return m_pipeline.GetTransactionPipe()->AddTransactionToProcess(connectionId, pTransaction, EPoolType::MEMPOOL) ? EStatus::SUCCESS : EStatus::UNKNOWN_ERROR;
			}
			case TxHashSetRequest:
			{
				const TxHashSetRequestMessage txHashSetRequestMessage = TxHashSetRequestMessage::Deserialize(byteBuffer);

				return SendTxHashSet(connectedPeer, socket, txHashSetRequestMessage);
			}
			case TxHashSetArchive:
			{
				const TxHashSetArchiveMessage txHashSetArchiveMessage = TxHashSetArchiveMessage::Deserialize(byteBuffer);

				return m_pipeline.GetTxHashSetPipe()->ReceiveTxHashSet(connectionId, socket, txHashSetArchiveMessage) ? EStatus::SUCCESS : EStatus::BAN_PEER;
			}
			case GetTransactionMsg:
			{
				const GetTransactionMessage getTransactionMessage = GetTransactionMessage::Deserialize(byteBuffer);
				const Hash& kernelHash = getTransactionMessage.GetKernelHash();

				TransactionPtr pTransaction = m_pBlockChainServer->GetTransactionByKernelHash(kernelHash);
				if (pTransaction != nullptr)
				{
					const TransactionMessage transactionMessage(pTransaction);
					return MessageSender(m_config).Send(socket, transactionMessage) ? EStatus::SUCCESS : EStatus::SOCKET_FAILURE;
				}

				return EStatus::RESOURCE_NOT_FOUND;
			}
			case TransactionKernelMsg:
			{
				if (m_pSyncStatus->GetStatus() != ESyncStatus::NOT_SYNCING)
				{
					return EStatus::SYNCING;
				}

				const TransactionKernelMessage transactionKernelMessage = TransactionKernelMessage::Deserialize(byteBuffer);
				const Hash& kernelHash = transactionKernelMessage.GetKernelHash();

				TransactionPtr pTransaction = m_pBlockChainServer->GetTransactionByKernelHash(kernelHash);
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

MessageProcessor::EStatus MessageProcessor::SendTxHashSet(
	ConnectedPeer& peer,
	Socket& socket,
	const TxHashSetRequestMessage& txHashSetRequestMessage)
{
	const time_t maxTxHashSetRequest = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() - std::chrono::hours(2));
	if (peer.GetPeer().GetLastTxHashSetRequest() > maxTxHashSetRequest)
	{
		LOG_WARNING_F("Peer ({}) requested multiple TxHashSet's within 2 hours.", socket.GetIPAddress());
		return EStatus::BAN_PEER;
	}

	LOG_INFO_F("Sending TxHashSet snapshot to {}", socket.GetIPAddress());
	peer.GetPeer().UpdateLastTxHashSetRequest();

	auto pHeader = m_pBlockChainServer->GetBlockHeaderByHash(txHashSetRequestMessage.GetBlockHash());
	if (pHeader == nullptr)
	{
		return EStatus::UNKNOWN_ERROR;
	}

	const std::string zipFilePath = m_pBlockChainServer->SnapshotTxHashSet(pHeader);
	if (zipFilePath.empty())
	{
		return EStatus::UNKNOWN_ERROR;
	}

	std::ifstream file(zipFilePath, std::ios::in | std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		return EStatus::UNKNOWN_ERROR;
	}

	const uint64_t fileSize = FileUtil::GetFileSize(zipFilePath);
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

		if (!sent || ShutdownManagerAPI::WasShutdownRequested())
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