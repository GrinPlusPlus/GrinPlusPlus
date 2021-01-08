#include "Connection.h"
#include "MessageRetriever.h"
#include "MessageProcessor.h"
#include "ConnectionManager.h"
#include "Messages/PingMessage.h"
#include "Messages/GetPeerAddressesMessage.h"
#include "Seed/HandShake.h"

#include <Net/SocketException.h>
#include <Common/Util/ThreadUtil.h>
#include <Common/Logger.h>
#include <thread>
#include <chrono>
#include <memory>

void Connection::Disconnect(const bool wait)
{
	m_terminate = true;
	if (wait) {
		ThreadUtil::Join(m_connectionThread);
		m_connectedPeer.GetPeer()->SetConnected(false);
		m_pSocket.reset();
	}
}

Connection::Ptr Connection::CreateInbound(
	const PeerPtr& pPeer,
	const SocketPtr& pSocket,
	const uint64_t connectionId,
	const Config& config,
	ConnectionManager& connectionManager,
	const std::weak_ptr<MessageProcessor>& pMessageProcessor,
	const SyncStatusConstPtr& pSyncStatus)
{
	auto pConnection = std::make_shared<Connection>(
		config,
		pSocket,
		connectionId,
		connectionManager,
		ConnectedPeer(pPeer, EDirection::INBOUND, pSocket->GetPort()),
		pSyncStatus,
		pMessageProcessor
	);
	pConnection->m_connectionThread = std::thread(Thread_ProcessConnection, pConnection);
	return pConnection;
}

Connection::Ptr Connection::CreateOutbound(
	const PeerPtr& pPeer,
	const uint64_t connectionId,
	const Config& config,
	ConnectionManager& connectionManager,
	const std::weak_ptr<MessageProcessor>& pMessageProcessor,
	const SyncStatusConstPtr& pSyncStatus)
{
	ConnectedPeer connectedPeer(pPeer, EDirection::OUTBOUND, config.GetP2PPort());
	SocketPtr pSocket = std::make_shared<Socket>(
		SocketAddress{ pPeer->GetIPAddress(), config.GetP2PPort() }
	);
	ConnectionPtr pConnection = std::make_shared<Connection>(
		config,
		pSocket,
		connectionId,
		connectionManager,
		connectedPeer,
		pSyncStatus,
		pMessageProcessor
	);

	pConnection->m_connectionThread = std::thread(Thread_ProcessConnection, pConnection);
	return pConnection;
}

bool Connection::IsConnectionActive() const
{
	if (m_terminate || m_pSocket == nullptr)
	{
		return false;
	}

	return m_pSocket->IsActive();
}

void Connection::AddToSendQueue(const IMessage& message)
{
	m_sendQueue.push_back(message.Clone());
}

bool Connection::ExceedsRateLimit() const
{
	return m_pSocket->GetRateCounter().GetSentInLastMinute() > 500
		|| m_pSocket->GetRateCounter().GetReceivedInLastMinute() > 500;
}

//
// Continuously checks for messages to send and/or receive until the connection is terminated.
// This function runs in its own thread.
//
void Connection::Thread_ProcessConnection(std::shared_ptr<Connection> pConnection)
{
	LoggerAPI::SetThreadName("PEER");

	try
	{
		pConnection->Connect();
	}
	catch (const std::exception& e)
	{
		if (pConnection->m_pSocket->IsSocketOpen()) {
			pConnection->m_pSocket->CloseSocket();
		}

        LOG_ERROR_F("Failed to connect: {}", e);
        pConnection->m_terminate = true;
		ThreadUtil::Detach(pConnection->m_connectionThread);
		return;
	}

	pConnection->GetPeer()->SetConnected(true);
	pConnection->Run();
	pConnection->GetSocket()->CloseSocket();
	pConnection->GetPeer()->SetConnected(false);
}

void Connection::Connect()
{
	EDirection direction = m_pSocket->IsSocketOpen() ? EDirection::INBOUND : EDirection::OUTBOUND;
	if (direction == EDirection::OUTBOUND)
	{
		m_pContext = std::make_shared<asio::io_context>();
		if (!m_pSocket->Connect(m_pContext)) {
			throw std::exception();
		}
	}

	HandShake handShake(m_config, m_connectionManager, m_pSyncStatus);
	if (!handShake.PerformHandshake(*m_pSocket, m_connectedPeer, direction)) {
		throw std::exception();
	}

	LOG_DEBUG("Successful Handshake");
	m_connectionManager.AddConnection(shared_from_this());
	SendMsg(GetPeerAddressesMessage(Capabilities::ECapability::FAST_SYNC_NODE));
}

void Connection::Run()
{
	auto lastPingTime = std::chrono::system_clock::now();
	auto lastReceivedMessageTime = std::chrono::system_clock::now();

	while (!m_terminate && !GetPeer()->IsBanned())
	{
		if (ExceedsRateLimit()) {
			LOG_WARNING_F("Banning peer ({}) for exceeding rate limit.", GetIPAddress());
			GetPeer()->Ban(EBanReason::Abusive);
			break;
		}

		auto now = std::chrono::system_clock::now();
		if (lastPingTime + std::chrono::seconds(10) < now) {
			uint64_t block_difficulty = m_pSyncStatus->GetBlockDifficulty();
			uint64_t block_height = m_pSyncStatus->GetBlockHeight();
			AddToSendQueue(PingMessage{ block_difficulty, block_height });

			lastPingTime = now;
		}

		try
		{
			bool messageSentOrReceived = false;

			// Check for received messages and if there is a new message, process it.
			std::unique_ptr<RawMessage> pRawMessage = MessageRetriever(m_config).RetrieveMessage(
				*m_pSocket,
				*GetPeer(),
				Socket::NON_BLOCKING
			);

			if (pRawMessage != nullptr) {
				auto pMessageProcessor = m_pMessageProcessor.lock();
				if (pMessageProcessor != nullptr) {
					pMessageProcessor->ProcessMessage(*this, *pRawMessage);
				}

				lastReceivedMessageTime = std::chrono::system_clock::now();
				messageSentOrReceived = true;
			}

			// Send the next message in the queue, if one exists.
			auto pMessageToSend = m_sendQueue.copy_front();
			if (pMessageToSend != nullptr) {
				IMessagePtr pMessage = *pMessageToSend;
				m_sendQueue.pop_front(1);
				SendMsg(*pMessage);
				messageSentOrReceived = true;
			}

			if (!messageSentOrReceived) {
				if ((lastReceivedMessageTime + std::chrono::seconds(30)) < std::chrono::system_clock::now()) {
					break;
				}

				ThreadUtil::SleepFor(std::chrono::milliseconds(5));
			}
		}
		catch (const DeserializationException&)
		{
			LOG_ERROR("Deserialization exception occurred");
			break;
		}
		catch (const SocketException&)
		{
#ifdef _WIN32
			const int lastError = WSAGetLastError();

			TCHAR* s = NULL;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&s, 0, NULL);

			const std::string errorMessage = StringUtil::ToUTF8(s);
			LOG_DEBUG("Socket exception occurred: " + errorMessage);

			LocalFree(s);
#endif
			break;
		}
		catch (const std::exception& e)
		{
			LOG_ERROR("Unknown exception occurred: " + std::string(e.what()));
			break;
		}
		catch (...)
		{
			LOG_ERROR("Unknown error occurred.");
			break;
		}
	}
}

bool Connection::SendMsg(const IMessage& message)
{
	std::vector<uint8_t> serialized_message = message.Serialize(GetProtocolVersion());
	if (message.GetMessageType() != MessageTypes::Ping && message.GetMessageType() != MessageTypes::Pong) {
		LOG_TRACE_F(
			"Sending {}b '{}' message to {}",
			serialized_message.size(),
			MessageTypes::ToString(message.GetMessageType()),
			GetSocket()
		);
	}

	return GetSocket()->Send(serialized_message, true);
}

void Connection::BanPeer(const EBanReason reason)
{
	LOG_WARNING_F("Banning peer {} for '{}'.", GetIPAddress(), BanReason::Format(reason));
	GetPeer()->Ban(reason);
	m_terminate = true;
}