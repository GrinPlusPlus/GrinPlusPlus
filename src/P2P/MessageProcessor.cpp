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
	ConnectionManager& connectionManager,
	Locked<PeerManager> peerManager,
	const IBlockChain::Ptr& pBlockChain,
	const std::shared_ptr<Pipeline>& pPipeline,
	SyncStatusConstPtr pSyncStatus)
	: m_connectionManager(connectionManager),
	m_peerManager(peerManager),
	m_pBlockChain(pBlockChain),
	m_pPipeline(pPipeline),
	m_pSyncStatus(pSyncStatus)
{

}

void MessageProcessor::ProcessMessage(const std::shared_ptr<Connection>& pConnection, const RawMessage& rawMessage)
{
	const EMessageType messageType = rawMessage.GetMessageHeader().GetMessageType();

	try
	{
		return ProcessMessageInternal(pConnection, rawMessage);
	}
	catch (const BadDataException& e)
	{
		LOG_ERROR_F(
			"{} from {} contained bad data: {}",
			MessageTypes::ToString(messageType),
			pConnection,
			e.what()
		);

		// TODO: Send ban reason
		pConnection->BanPeer(e.GetReason());
	}
	catch (const BlockChainException& e)
	{
		LOG_ERROR_F(
			"BlockChain exception while processing {} from {}: {}",
			MessageTypes::ToString(messageType),
			pConnection,
			e.what()
		);
	}
	catch (const DeserializationException&)
	{
		LOG_ERROR_F(
			"Deserialization exception while processing {} from {}",
			MessageTypes::ToString(messageType),
			pConnection
		);
		pConnection->Disconnect();
	}
}

void MessageProcessor::ProcessMessageInternal(const std::shared_ptr<Connection>& pConnection, const RawMessage& rawMessage)
{
	const MessageHeader& header = rawMessage.GetMessageHeader();
	EProtocolVersion protocolVersion = pConnection->GetProtocolVersion();
	ByteBuffer byteBuffer(rawMessage.GetPayload(), protocolVersion);

	switch (header.GetMessageType())
	{
		case Error:
		{
			const ErrorMessage errorMessage = ErrorMessage::Deserialize(byteBuffer);
			LOG_WARNING_F("Error message \"{}\" retrieved from {}", errorMessage.GetErrorMessage(), pConnection);
			break;
		}
		case BanReasonMsg:
		{
			const BanReasonMessage banReasonMessage = BanReasonMessage::Deserialize(byteBuffer);
			std::string banMessage = BanReason::Format((EBanReason)banReasonMessage.GetBanReason());
			LOG_WARNING_F("BanReason message '{}' retrieved from {}", banMessage, pConnection);
			break;
		}
		case Ping:
		{
			const PingMessage pingMessage = PingMessage::Deserialize(byteBuffer);
			pConnection->UpdateTotals(pingMessage.GetTotalDifficulty(), pingMessage.GetHeight());

			auto pTipHeader = m_pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED);
			const PongMessage pongMessage(pTipHeader->GetTotalDifficulty(), pTipHeader->GetHeight());

			pConnection->SendAsync(pongMessage);
			break;
		}
		case Pong:
		{
			const PongMessage pongMessage = PongMessage::Deserialize(byteBuffer);
			pConnection->UpdateTotals(pongMessage.GetTotalDifficulty(), pongMessage.GetHeight());
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
				[this](const PeerPtr& peer) { return SocketAddress(peer->GetIPAddress(), Global::GetConfig().GetP2PPort()); }
			);

			LOG_TRACE_F("Sending {} addresses to {}.", socketAddresses.size(), pConnection);
			pConnection->SendAsync(PeerAddressesMessage{ std::move(socketAddresses) });
			break;
		}
		case PeerAddrs:
		{
			const PeerAddressesMessage peerAddressesMessage = PeerAddressesMessage::Deserialize(byteBuffer);
			const std::vector<SocketAddress>& peerAddresses = peerAddressesMessage.GetPeerAddresses();

			LOG_TRACE_F("Received {} addresses from {}.", peerAddresses.size(), pConnection);
			m_peerManager.Write()->AddFreshPeers(peerAddresses);
			break;
		}
		case GetHeaders:
		{
			const GetHeadersMessage getHeadersMessage = GetHeadersMessage::Deserialize(byteBuffer);
			const std::vector<Hash>& hashes = getHeadersMessage.GetHashes();

			std::vector<BlockHeaderPtr> blockHeaders = BlockLocator(m_pBlockChain).LocateHeaders(hashes);
			LOG_DEBUG_F("Sending {} headers to {}.", blockHeaders.size(), pConnection);
                
			pConnection->SendAsync(HeadersMessage{ std::move(blockHeaders) });
			break;
		}
		case Header:
		{
			auto pBlockHeader = HeaderMessage::Deserialize(byteBuffer).GetHeader();

			if (pBlockHeader->GetTotalDifficulty() > pConnection->GetTotalDifficulty()) {
				pConnection->UpdateTotals(pBlockHeader->GetTotalDifficulty(), pBlockHeader->GetHeight());
			}

			if (m_pSyncStatus->GetStatus() != ESyncStatus::NOT_SYNCING) {
				break;
			}

			const EBlockChainStatus status = m_pBlockChain->AddBlockHeader(pBlockHeader);
			if (status == EBlockChainStatus::SUCCESS || status == EBlockChainStatus::ALREADY_EXISTS || status == EBlockChainStatus::ORPHANED) {
				if (!m_pBlockChain->HasBlock(pBlockHeader->GetHeight(), pBlockHeader->GetHash())) {
					LOG_TRACE_F("Valid header {} received from {}. Requesting compact block", *pBlockHeader, pConnection);
					const GetCompactBlockMessage getCompactBlockMessage(pBlockHeader->GetHash());
					pConnection->SendAsync(GetCompactBlockMessage{ pBlockHeader->GetHash() });
				}
			} else if (status == EBlockChainStatus::INVALID) {
				pConnection->BanPeer(EBanReason::BadBlockHeader);
			} else {
				LOG_TRACE_F("Header {} from {} not needed", *pBlockHeader, pConnection);
			}

			break;
		}
		case Headers:
		{
			const HeadersMessage headersMessage = HeadersMessage::Deserialize(byteBuffer);
			const std::vector<BlockHeaderPtr>& blockHeaders = headersMessage.GetHeaders();

			LOG_DEBUG_F("{} headers received from {}", blockHeaders.size(), pConnection);

			const EBlockChainStatus status = m_pBlockChain->AddBlockHeaders(blockHeaders);
			if (status == EBlockChainStatus::INVALID) {
				pConnection->BanPeer(EBanReason::BadBlockHeader);
			}

			LOG_DEBUG_F("Headers message from {} finished processing", pConnection);
			break;
		}
		case GetBlock:
		{
			const GetBlockMessage getBlockMessage = GetBlockMessage::Deserialize(byteBuffer);
			std::unique_ptr<FullBlock> pBlock = m_pBlockChain->GetBlockByHash(getBlockMessage.GetHash());
			if (pBlock != nullptr) {
				pConnection->SendAsync(BlockMessage{ std::move(*pBlock) });
			}

			break;
		}
		case Block:
		{
			const BlockMessage blockMessage = BlockMessage::Deserialize(byteBuffer);
			const FullBlock& block = blockMessage.GetBlock();

			LOG_TRACE_F("Block received: {}", block.GetHeight());

			if (m_pSyncStatus->GetStatus() == ESyncStatus::SYNCING_BLOCKS) {
				m_pPipeline->ProcessBlock(*pConnection, block);
			} else {
				const EBlockChainStatus added = m_pBlockChain->AddBlock(block);
				if (added == EBlockChainStatus::SUCCESS) {
					const HeaderMessage headerMessage(block.GetHeader());
					m_connectionManager.BroadcastMessage(headerMessage, pConnection->GetId());
				} else if (added == EBlockChainStatus::ORPHANED) {
					if (block.GetTotalDifficulty() > m_pBlockChain->GetTotalDifficulty(EChainType::CONFIRMED))
					{
						pConnection->SendAsync(GetCompactBlockMessage{ block.GetPreviousHash() });
					}
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
				pConnection->SendAsync(CompactBlockMessage{ *pCompactBlock });
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
				m_connectionManager.BroadcastMessage(headerMessage, pConnection->GetId());
				break;
			}
			else if (added == EBlockChainStatus::TRANSACTIONS_MISSING)
			{
				pConnection->SendAsync(GetBlockMessage{ compactBlock.GetHash() });
				break;
			}
			else if (added == EBlockChainStatus::ORPHANED)
			{
				if (compactBlock.GetHeight() < (m_pBlockChain->GetHeight(EChainType::CONFIRMED) + 100))
				{
					if (compactBlock.GetTotalDifficulty() > m_pBlockChain->GetTotalDifficulty(EChainType::CONFIRMED))
					{
						pConnection->SendAsync(GetCompactBlockMessage{ compactBlock.GetPreviousHash() });
					}
				}
			}

			break;
		}
		case StemTransaction:
		{
			if (m_pSyncStatus->GetStatus() == ESyncStatus::NOT_SYNCING) {
				TransactionPtr pTransaction = StemTransactionMessage::Deserialize(byteBuffer).GetTransaction();
				m_pPipeline->ProcessTransaction(*pConnection, pTransaction, EPoolType::STEMPOOL);
			}

			break;
		}
		case TransactionMsg:
		{
			if (m_pSyncStatus->GetStatus() == ESyncStatus::NOT_SYNCING) {
				TransactionPtr pTransaction = TransactionMessage::Deserialize(byteBuffer).GetTransaction();
				m_pPipeline->ProcessTransaction(*pConnection, pTransaction, EPoolType::MEMPOOL);
			}

			break;
		}
		case TxHashSetRequest:
		{
			TxHashSetRequestMessage msg = TxHashSetRequestMessage::Deserialize(byteBuffer);
			m_pPipeline->SendTxHashSet(pConnection, msg.GetBlockHash());
			break;
		}
		case TxHashSetArchive:
		{
			m_pPipeline->ReceiveTxHashSet(pConnection, TxHashSetArchiveMessage::Deserialize(byteBuffer));
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
				pConnection->SendAsync(TransactionMessage{ pTransaction });
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
				pConnection->SendAsync(GetTransactionMessage{ std::move(kernelHash) });
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