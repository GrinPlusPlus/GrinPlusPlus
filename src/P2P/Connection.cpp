#include "Connection.h"
#include "MessageRetriever.h"
#include "MessageProcessor.h"
#include "MessageSender.h"
#include "ConnectionManager.h"
#include "Messages/PingMessage.h"
#include "Messages/GetPeerAddressesMessage.h"
#include "Seed/HandShake.h"

#include <Net/SocketException.h>
#include <Common/Util/ThreadUtil.h>
#include <Infrastructure/ThreadManager.h>
#include <Infrastructure/Logger.h>
#include <thread>
#include <chrono>
#include <memory>

Connection::Connection(
	SocketPtr pSocket,
	const uint64_t connectionId,
	const Config& config,
	ConnectionManager& connectionManager,
	Locked<PeerManager> peerManager,
	IBlockChainServerPtr pBlockChainServer,
	const ConnectedPeer& connectedPeer,
	Pipeline& pipeline,
	SyncStatusConstPtr pSyncStatus)
	: m_pSocket(pSocket),
	m_connectionId(connectionId),
	m_config(config),
	m_connectionManager(connectionManager),
	m_peerManager(peerManager),
	m_pBlockChainServer(pBlockChainServer),
	m_connectedPeer(connectedPeer),
	m_pipeline(pipeline),
	m_pSyncStatus(pSyncStatus),
	m_terminate(false)
{

}

Connection::~Connection()
{
	Disconnect();
}

void Connection::Disconnect()
{
	m_terminate = true;
	ThreadUtil::Join(m_connectionThread);
	m_pSocket.reset();
}

std::shared_ptr<Connection> Connection::Create(
	SocketPtr pSocket,
	const uint64_t connectionId,
	const Config& config,
	ConnectionManager& connectionManager,
	Locked<PeerManager> peerManager,
	IBlockChainServerPtr pBlockChainServer,
	const ConnectedPeer& connectedPeer,
	std::shared_ptr<Pipeline> pPipeline,
	SyncStatusConstPtr pSyncStatus)
{
	auto pConnection = std::shared_ptr<Connection>(new Connection(
		pSocket,
		connectionId,
		config,
		connectionManager,
		peerManager,
		pBlockChainServer,
		connectedPeer,
		*pPipeline,
		pSyncStatus
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

void Connection::Send(const IMessage& message)
{
	std::lock_guard<std::mutex> lockGuard(m_sendMutex);
	m_sendQueue.emplace(message.Clone());
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
			HandShake handshake(pConnection->m_config, pConnection->m_connectionManager, pConnection->m_pBlockChainServer);
			handshakeSuccess = handshake.PerformHandshake(
				*pConnection->m_pSocket,
				pConnection->m_connectedPeer,
				direction
			);
		}

		if (handshakeSuccess)
		{
			LOG_DEBUG("Successful Handshake");
			pConnection->m_connectionManager.AddConnection(pConnection);
			if (pConnection->m_peerManager.Read()->ArePeersNeeded(Capabilities::ECapability::FAST_SYNC_NODE))
			{
				const Capabilities capabilites(Capabilities::ECapability::FAST_SYNC_NODE);
				const GetPeerAddressesMessage getPeerAddressesMessage(capabilites);
				pConnection->Send(getPeerAddressesMessage);
			}
		}
		else
		{
			pConnection->m_pSocket->CloseSocket();
			pConnection->m_terminate = true;
			pConnection->m_connectionThread.detach();
			return;
		}
	}
	catch (...)
	{
        LOG_ERROR("Exception caught");
        pConnection->m_terminate = true;
		pConnection->m_connectionThread.detach();
		return;
	}

	pConnection->m_peerManager.Write()->SetPeerConnected(pConnection->GetConnectedPeer().GetPeer(), true);

	MessageProcessor messageProcessor(
		pConnection->m_config,
		pConnection->m_connectionManager,
		pConnection->m_peerManager,
		pConnection->m_pBlockChainServer,
		pConnection->m_pipeline,
		pConnection->m_pSyncStatus
	);
	const MessageRetriever messageRetriever(
		pConnection->m_config,
		pConnection->m_connectionManager
	);

	SyncStatusConstPtr pSyncStatus = pConnection->m_pSyncStatus;

	std::chrono::system_clock::time_point lastPingTime = std::chrono::system_clock::now();
	std::chrono::system_clock::time_point lastReceivedMessageTime = std::chrono::system_clock::now();

	while (!pConnection->m_terminate)
	{
		auto now = std::chrono::system_clock::now();
		if (lastPingTime + std::chrono::seconds(10) < now)
		{
			const PingMessage pingMessage(pSyncStatus->GetBlockDifficulty(), pSyncStatus->GetBlockHeight());
			pConnection->Send(pingMessage);

			lastPingTime = now;
		}

		std::unique_lock<std::mutex> lockGuard(pConnection->m_peerMutex);

		try
		{
			bool messageSentOrReceived = false;

			// Check for received messages and if there is a new message, process it.
			std::unique_ptr<RawMessage> pRawMessage = messageRetriever.RetrieveMessage(
				*pConnection->m_pSocket,
				pConnection->m_connectedPeer,
				MessageRetriever::NON_BLOCKING
			);

			if (pRawMessage.get() != nullptr)
			{
				const MessageProcessor::EStatus status = messageProcessor.ProcessMessage(
					pConnection->m_connectionId,
					*pConnection->m_pSocket,
					pConnection->m_connectedPeer,
					*pRawMessage
				);

                if (status == MessageProcessor::EStatus::BAN_PEER)
                {
                    pConnection->m_connectionManager.BanConnection(pConnection->m_connectionId, EBanReason::BadBlock); // TODO: Determine real reason.
                }
                
                lastReceivedMessageTime = std::chrono::system_clock::now();
				messageSentOrReceived = true;
			}

			// Send the next message in the queue, if one exists.
			std::unique_lock<std::mutex> sendLockGuard(pConnection->m_sendMutex);
			if (!pConnection->m_sendQueue.empty())
			{
				IMessagePtr pMessageToSend(pConnection->m_sendQueue.front());
				pConnection->m_sendQueue.pop();
				sendLockGuard.unlock();

				MessageSender(pConnection->m_config).Send(*pConnection->m_pSocket, *pMessageToSend);

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

			const std::string errorMessage = s;
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

	pConnection->m_peerManager.Write()->SetPeerConnected(pConnection->GetConnectedPeer().GetPeer(), false);
}
