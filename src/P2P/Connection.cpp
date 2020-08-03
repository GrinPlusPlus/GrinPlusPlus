#include "Connection.h"
#include "MessageRetriever.h"
#include "MessageProcessor.h"
#include "ConnectionManager.h"
#include "Messages/PingMessage.h"
#include "Messages/GetPeerAddressesMessage.h"
#include "Seed/HandShake.h"

#include <Net/SocketException.h>
#include <Common/Util/ThreadUtil.h>
#include <Common/ThreadManager.h>
#include <Common/Logger.h>
#include <thread>
#include <chrono>
#include <memory>

Connection::Connection(
	const Config& config,
	const SocketPtr& pSocket,
	const uint64_t connectionId,
	ConnectionManager& connectionManager,
	const ConnectedPeer& connectedPeer,
	SyncStatusConstPtr pSyncStatus,
	std::shared_ptr<HandShake> pHandShake,
	const std::weak_ptr<MessageProcessor>& pMessageProcessor)
	: m_config(config),
	m_pSocket(pSocket),
	m_connectionId(connectionId),
	m_connectionManager(connectionManager),
	m_connectedPeer(connectedPeer),
	m_pSyncStatus(pSyncStatus),
	m_pHandShake(pHandShake),
	m_pMessageProcessor(pMessageProcessor),
	m_terminate(false)
{

}

Connection::~Connection()
{
	Disconnect(true);
}

void Connection::Disconnect(const bool wait)
{
	m_terminate = true;
	if (wait) {
		ThreadUtil::Join(m_connectionThread);
		m_connectedPeer.GetPeer()->SetConnected(false);
		m_pSocket.reset();
	}
}

std::shared_ptr<Connection> Connection::Create(
	const SocketPtr& pSocket,
	const uint64_t connectionId,
	const Config& config,
	ConnectionManager& connectionManager,
	IBlockChainServerPtr pBlockChainServer,
	const ConnectedPeer& connectedPeer,
	const std::weak_ptr<MessageProcessor>& pMessageProcessor,
	SyncStatusConstPtr pSyncStatus)
{
	auto pHandShake = std::make_shared<HandShake>(config, connectionManager, pBlockChainServer);

	auto pConnection = std::shared_ptr<Connection>(new Connection(
		config,
		pSocket,
		connectionId,
		connectionManager,
		connectedPeer,
		pSyncStatus,
		pHandShake,
		pMessageProcessor
	));
	pConnection->m_connectionThread = std::thread(Thread_ProcessConnection, pConnection);
	ThreadManagerAPI::SetThreadName(pConnection->m_connectionThread.get_id(), "PEER");
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
	try
	{
		EDirection direction = EDirection::INBOUND;
		bool connected = pConnection->GetSocket()->IsSocketOpen();
		if (!connected)
		{
			pConnection->m_pContext = std::make_shared<asio::io_context>();
			direction = EDirection::OUTBOUND;
			connected = pConnection->m_pSocket->Connect(pConnection->m_pContext);
		}

		bool handshakeSuccess = false;
		if (connected)
		{
			handshakeSuccess = pConnection->m_pHandShake->PerformHandshake(
				*pConnection->m_pSocket,
				pConnection->m_connectedPeer,
				direction
			);
		}

		if (handshakeSuccess)
		{
			LOG_DEBUG("Successful Handshake");
			pConnection->m_connectionManager.AddConnection(pConnection);
			pConnection->SendMsg(GetPeerAddressesMessage(Capabilities::ECapability::FAST_SYNC_NODE));
		}
		else
		{
			pConnection->m_pSocket->CloseSocket();
			pConnection->m_terminate = true;
			ThreadUtil::Detach(pConnection->m_connectionThread);
			return;
		}
	}
	catch (...)
	{
        LOG_ERROR("Exception caught");
        pConnection->m_terminate = true;
		ThreadUtil::Detach(pConnection->m_connectionThread);
		return;
	}

	pConnection->m_connectedPeer.GetPeer()->SetConnected(true);

	SyncStatusConstPtr pSyncStatus = pConnection->m_pSyncStatus;

	auto lastPingTime = std::chrono::system_clock::now();
	auto lastReceivedMessageTime = std::chrono::system_clock::now();

	while (!pConnection->m_terminate && !pConnection->GetPeer()->IsBanned())
	{
		if (pConnection->ExceedsRateLimit())
		{
			LOG_WARNING_F("Banning peer ({}) for exceeding rate limit.", pConnection->GetIPAddress());
			pConnection->GetPeer()->Ban(EBanReason::Abusive);
			break;
		}

		auto now = std::chrono::system_clock::now();
		if (lastPingTime + std::chrono::seconds(10) < now)
		{
			const PingMessage pingMessage(pSyncStatus->GetBlockDifficulty(), pSyncStatus->GetBlockHeight());
			pConnection->AddToSendQueue(pingMessage);

			lastPingTime = now;
		}

		try
		{
			bool messageSentOrReceived = false;

			// Check for received messages and if there is a new message, process it.
			std::unique_ptr<RawMessage> pRawMessage = MessageRetriever(pConnection->m_config).RetrieveMessage(
				*pConnection->m_pSocket,
				*pConnection->GetPeer(),
				Socket::NON_BLOCKING
			);

			if (pRawMessage != nullptr)
			{
				auto pMessageProcessor = pConnection->m_pMessageProcessor.lock();
				if (pMessageProcessor != nullptr)
				{
					pMessageProcessor->ProcessMessage(*pConnection, *pRawMessage);
				}
                
                lastReceivedMessageTime = std::chrono::system_clock::now();
				messageSentOrReceived = true;
			}

			// Send the next message in the queue, if one exists.
			auto pMessageToSend = pConnection->m_sendQueue.copy_front();
			if (pMessageToSend != nullptr)
			{
				IMessagePtr pMessage = *pMessageToSend;
				pConnection->m_sendQueue.pop_front(1);
				pConnection->SendMsg(*pMessage);
				messageSentOrReceived = true;
			}
			
			if (!messageSentOrReceived)
			{
				if ((lastReceivedMessageTime + std::chrono::seconds(30)) < std::chrono::system_clock::now())
				{
					break;
				}

				ThreadUtil::SleepFor(std::chrono::milliseconds(5), pConnection->m_terminate);
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

	pConnection->GetSocket()->CloseSocket();
	pConnection->m_connectedPeer.GetPeer()->SetConnected(false);
}

bool Connection::SendMsg(const IMessage& message)
{
	std::vector<uint8_t> serialized_message = message.Serialize(
		m_config.GetEnvironment(),
		GetProtocolVersion()
	);
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