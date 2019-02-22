#include "Connection.h"
#include "MessageRetriever.h"
#include "MessageProcessor.h"
#include "MessageSender.h"
#include "ConnectionManager.h"
#include "Seed/PeerManager.h"

#include <Infrastructure/ThreadManager.h>
#include <Infrastructure/Logger.h>
#include <thread>
#include <chrono>
#include <memory>

Connection::Connection(const uint64_t connectionId, const Config& config, ConnectionManager& connectionManager, PeerManager& peerManager, IBlockChainServer& blockChainServer, const ConnectedPeer& connectedPeer)
	: m_connectionId(connectionId), m_config(config), m_connectionManager(connectionManager), m_peerManager(peerManager), m_blockChainServer(blockChainServer), m_connectedPeer(connectedPeer)
{

}

bool Connection::Connect()
{
	if (IsConnectionActive())
	{
		return true;
	}

	m_terminate = true;
	if (m_connectionThread.joinable())
	{
		m_connectionThread.join();
	}

	m_terminate = false;

	m_connectionThread = std::thread(Thread_ProcessConnection, std::ref(*this));
	ThreadManagerAPI::SetThreadName(m_connectionThread.get_id(), "PEER_CONNECTION");

	return true;
}

bool Connection::IsConnectionActive() const
{
	if (m_terminate)
	{
		return false;
	}

	const time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	const time_t lastContactTime = m_connectedPeer.GetPeer().GetLastContactTime();
	const double differenceInSeconds = std::difftime(currentTime, lastContactTime);
	
	return differenceInSeconds < 30.0;
}

void Connection::Disconnect()
{
	m_terminate = true;

	if (m_connectionThread.joinable())
	{
		m_connectionThread.join();
	}
}

void Connection::Send(const IMessage& message)
{
	std::lock_guard<std::mutex> lockGuard(m_sendMutex);
	m_sendQueue.emplace(message.Clone());
}

//
// Continuously checks for messages to send and/or receive until the connection is terminated.
// This function runs in its own thread.
//
void Connection::Thread_ProcessConnection(Connection& connection)
{
	MessageProcessor messageProcessor(connection.m_config, connection.m_connectionManager, connection.m_peerManager, connection.m_blockChainServer);
	const MessageRetriever messageRetriever(connection.m_config);

	while (!connection.m_terminate)
	{
		std::unique_lock<std::mutex> lockGuard(connection.m_peerMutex);

		bool messageSentOrReceived = false;

		try
		{
			// Check for received messages and if there is a new message, process it.
			std::unique_ptr<RawMessage> pRawMessage = messageRetriever.RetrieveMessage(connection.m_connectedPeer, MessageRetriever::NON_BLOCKING);
			if (pRawMessage.get() != nullptr)
			{
				const MessageProcessor::EStatus status = messageProcessor.ProcessMessage(connection.m_connectionId, connection.m_connectedPeer, *pRawMessage);
				messageSentOrReceived = true;
			}
		}
		catch (const DeserializationException&)
		{
			LoggerAPI::LogError("Connection::Thread_ProcessConnection - Deserialization exception occurred.");
			break;
		}

		// Send the next message in the queue, if one exists.
		std::unique_lock<std::mutex> sendLockGuard(connection.m_sendMutex);
		if (!connection.m_sendQueue.empty())
		{
			std::unique_ptr<IMessage> pMessageToSend(connection.m_sendQueue.front());
			connection.m_sendQueue.pop();

			MessageSender(connection.m_config).Send(connection.m_connectedPeer, *pMessageToSend);

			messageSentOrReceived = true;
		}

		sendLockGuard.unlock();
		lockGuard.unlock();

		if (!messageSentOrReceived)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
	}

	closesocket(connection.GetConnectedPeer().GetSocket());
}