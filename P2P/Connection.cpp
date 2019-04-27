#include "Connection.h"
#include "MessageRetriever.h"
#include "MessageProcessor.h"
#include "MessageSender.h"
#include "ConnectionManager.h"
#include "Seed/PeerManager.h"
#include "Messages/PingMessage.h"

#include <Net/SocketException.h>
#include <Common/Util/ThreadUtil.h>
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

	if (!m_connectedPeer.GetSocket().IsConnected())
	{
		return false;
	}

	return true;

	//const time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	//const time_t lastContactTime = m_connectedPeer.GetPeer().GetLastContactTime();
	//const double differenceInSeconds = std::difftime(currentTime, lastContactTime);
	//
	//return differenceInSeconds < 120.0;
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
	connection.m_peerManager.SetPeerConnected(connection.GetConnectedPeer().GetPeer(), true);

	MessageProcessor messageProcessor(connection.m_config, connection.m_connectionManager, connection.m_peerManager, connection.m_blockChainServer);
	const MessageRetriever messageRetriever(connection.m_config, connection.m_connectionManager);

	const SyncStatus& syncStatus = connection.m_connectionManager.GetSyncStatus();

	std::chrono::system_clock::time_point lastPingTime = std::chrono::system_clock::now();
	std::chrono::system_clock::time_point lastReceivedMessageTime = std::chrono::system_clock::now();

	while (!connection.m_terminate)
	{
		auto now = std::chrono::system_clock::now();
		if (lastPingTime + std::chrono::seconds(10) < now)
		{
			const PingMessage pingMessage(syncStatus.GetBlockDifficulty(), syncStatus.GetBlockHeight());
			connection.Send(pingMessage);

			lastPingTime = now;
		}

		std::unique_lock<std::mutex> lockGuard(connection.m_peerMutex);

		try
		{
			bool messageSentOrReceived = false;

			// Check for received messages and if there is a new message, process it.
			std::unique_ptr<RawMessage> pRawMessage = messageRetriever.RetrieveMessage(connection.m_connectedPeer, MessageRetriever::NON_BLOCKING);
			if (pRawMessage.get() != nullptr)
			{
				const MessageProcessor::EStatus status = messageProcessor.ProcessMessage(connection.m_connectionId, connection.m_connectedPeer, *pRawMessage);
				lastReceivedMessageTime = std::chrono::system_clock::now();
				messageSentOrReceived = true;
			}

			// Send the next message in the queue, if one exists.
			std::unique_lock<std::mutex> sendLockGuard(connection.m_sendMutex);
			if (!connection.m_sendQueue.empty())
			{
				std::unique_ptr<IMessage> pMessageToSend(connection.m_sendQueue.front());
				connection.m_sendQueue.pop();
				sendLockGuard.unlock();

				MessageSender(connection.m_config).Send(connection.m_connectedPeer, *pMessageToSend);

				messageSentOrReceived = true;
			}
			else
			{
				sendLockGuard.unlock();
			}

			lockGuard.unlock();

			if (!messageSentOrReceived)
			{
				if ((lastReceivedMessageTime + std::chrono::seconds(30)) < std::chrono::system_clock::now())
				{
					break;
				}

				ThreadUtil::SleepFor(std::chrono::milliseconds(5), connection.m_terminate);
			}
		}
		catch (const DeserializationException&)
		{
			LoggerAPI::LogError("Connection::Thread_ProcessConnection - Deserialization exception occurred.");
			break;
		}
		catch (const SocketException&)
		{
			const int lastError = WSAGetLastError();

			TCHAR* s = NULL;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&s, 0, NULL);

			const std::string errorMessage = s;
			LoggerAPI::LogDebug("Connection::Thread_ProcessConnection - Socket exception occurred: " + errorMessage);

			LocalFree(s);

			break;
		}
		catch (const std::exception& e)
		{
			LoggerAPI::LogError("Connection::Thread_ProcessConnection - Unknown exception occurred: " + std::string(e.what()));
			break;
		}
	}

	connection.GetConnectedPeer().GetSocket().CloseSocket();

	connection.m_peerManager.SetPeerConnected(connection.GetConnectedPeer().GetPeer(), false);
}