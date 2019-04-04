#include "ConnectionFactory.h"

#include "../Messages/HandMessage.h"
#include "../Messages/ShakeMessage.h"
#include "../Messages/BanReasonMessage.h"
#include "../MessageRetriever.h"
#include "../MessageSender.h"
#include "../ConnectionManager.h"
#include "PeerManager.h"

#include <Net/SocketFactory.h>
#include <Net/SocketException.h>
#include <Common/Util/VectorUtil.h>
#include <Crypto/RandomNumberGenerator.h>
#include <Infrastructure/Logger.h>
#include <ctime>
#include <cstdlib>

static const uint64_t NONCE = RandomNumberGenerator::GenerateRandom(0, UINT64_MAX);

ConnectionFactory::ConnectionFactory(const Config& config, ConnectionManager& connectionManager, PeerManager& peerManager, IBlockChainServer& blockChainServer)
	: m_config(config), m_connectionManager(connectionManager), m_peerManager(peerManager), m_blockChainServer(blockChainServer)
{

}

Connection* ConnectionFactory::CreateConnection(Peer& peer, const EDirection direction, const std::optional<Socket>& socketOpt) const
{
	std::optional<Socket> socketOptional = socketOpt;
	if (direction == EDirection::OUTBOUND)
	{
		socketOptional = SocketFactory::Connect(peer.GetIPAddress(), m_config.GetEnvironment().GetP2PPort() /*peer.GetPortNumber()*/); // TODO: Use actual port number
	}

	if (socketOptional.has_value())
	{
		Socket socket = socketOptional.value();
		try
		{
			LoggerAPI::LogTrace("ConnectionFactory::CreateConnection - Performing handshake with " + socket.GetSocketAddress().Format());

			Connection* pConnection = PerformHandshake(socket, peer, direction);
			if (pConnection != nullptr)
			{
				LoggerAPI::LogDebug("ConnectionFactory::CreateConnection - Successfully connected to " + socket.GetSocketAddress().Format());
				// This will start the message processing loop.
				pConnection->Connect();

				return pConnection;
			}
		}
		catch (const DeserializationException&)
		{
			LoggerAPI::LogDebug("ConnectionFactory::CreateConnection - Failed to deserialize handshake from " + socket.GetSocketAddress().Format());
		}
		catch (const SocketException&)
		{
			LoggerAPI::LogDebug("ConnectionFactory::CreateConnection - Socket exception encountered with " + socket.GetSocketAddress().Format());
		}

		socket.CloseSocket();
	}

	return nullptr;
}

Connection* ConnectionFactory::PerformHandshake(Socket& connection, Peer& peer, const EDirection direction) const
{
	ConnectedPeer connectedPeer(connection, peer, direction);
	if (direction == EDirection::OUTBOUND)
	{
		return PerformOutboundHandshake(connectedPeer);
	}
	else
	{
		// Get Hand Message
		std::unique_ptr<RawMessage> pReceivedMessage = MessageRetriever(m_config, m_connectionManager).RetrieveMessage(connectedPeer, MessageRetriever::BLOCKING);
		if (pReceivedMessage != nullptr)
		{
			if (pReceivedMessage->GetMessageHeader().GetMessageType() == MessageTypes::Hand)
			{
				ByteBuffer byteBuffer(pReceivedMessage->GetPayload());
				const HandMessage handMessage = HandMessage::Deserialize(byteBuffer);

				if (handMessage.GetNonce() != NONCE && !m_connectionManager.IsConnected(peer.GetIPAddress()))
				{
					connectedPeer.UpdateVersion(handMessage.GetVersion());
					connectedPeer.UpdateCapabilities(handMessage.GetCapabilities());
					connectedPeer.UpdateUserAgent(handMessage.GetUserAgent());
					connectedPeer.UpdateTotals(handMessage.GetTotalDifficulty(), 0);

					// Send Shake Message
					if (TransmitShakeMessage(connectedPeer))
					{
						return new Connection(m_nextId++, m_config, m_connectionManager, m_peerManager, m_blockChainServer, connectedPeer);
					}
					else
					{
						LoggerAPI::LogDebug("ConnectionFactory::PerformHandshake - Failed to transmit shake message to " + connectedPeer.GetSocket().GetSocketAddress().Format());
						return nullptr;
					}
				}
			}
			else
			{
				LoggerAPI::LogDebug("ConnectionFactory::PerformHandshake - First message from " + connectedPeer.GetSocket().GetSocketAddress().Format() 
					+ " was of type " + std::to_string(pReceivedMessage->GetMessageHeader().GetMessageType()));
				return nullptr;
			}
		}

	}

	LoggerAPI::LogTrace("ConnectionFactory::PerformHandshake - Unable to connect to " + connectedPeer.GetSocket().GetSocketAddress().Format());
	return nullptr;
}

Connection* ConnectionFactory::PerformOutboundHandshake(ConnectedPeer& connectedPeer) const
{
	// Send Hand Message
	const bool bHandMessageSent = TransmitHandMessage(connectedPeer);
	if (bHandMessageSent)
	{
		// Get Shake Message
		std::unique_ptr<RawMessage> pReceivedMessage = MessageRetriever(m_config, m_connectionManager).RetrieveMessage(connectedPeer, MessageRetriever::BLOCKING);

		if (pReceivedMessage.get() != nullptr)
		{
			const MessageTypes::EMessageType messageType = pReceivedMessage->GetMessageHeader().GetMessageType();
			if (messageType == MessageTypes::Shake)
			{
				ByteBuffer byteBuffer(pReceivedMessage->GetPayload());
				const ShakeMessage shakeMessage = ShakeMessage::Deserialize(byteBuffer);

				connectedPeer.UpdateVersion(shakeMessage.GetVersion());
				connectedPeer.UpdateCapabilities(shakeMessage.GetCapabilities());
				connectedPeer.UpdateUserAgent(shakeMessage.GetUserAgent());
				connectedPeer.UpdateTotals(shakeMessage.GetTotalDifficulty(), 0);

				return new Connection(m_nextId++, m_config, m_connectionManager, m_peerManager, m_blockChainServer, connectedPeer);
			}
			else if (messageType == MessageTypes::BanReasonMsg)
			{
				ByteBuffer byteBuffer(pReceivedMessage->GetPayload());
				const BanReasonMessage banReasonMessage = BanReasonMessage::Deserialize(byteBuffer);

				LoggerAPI::LogDebug("ConnectionFactory::PerformOutboundHandshake - Ban message received from " + connectedPeer.GetSocket().GetSocketAddress().Format() + " with reason: " + std::to_string(banReasonMessage.GetBanReason()));
				return nullptr;
			}
			else
			{
				LoggerAPI::LogDebug("ConnectionFactory::PerformOutboundHandshake - Expected shake from " + connectedPeer.GetSocket().GetSocketAddress().Format() + " but received " + MessageTypes::ToString(messageType));
				return nullptr;
			}
		}

		LoggerAPI::LogTrace("ConnectionFactory::PerformOutboundHandshake - Shake message not received from " + connectedPeer.GetSocket().GetSocketAddress().Format());
		return nullptr;
	}

	LoggerAPI::LogDebug("ConnectionFactory::PerformOutboundHandshake - Hand message not sent to " + connectedPeer.GetSocket().GetSocketAddress().Format());
	return nullptr;
}

bool ConnectionFactory::TransmitHandMessage(ConnectedPeer& connectedPeer) const
{
	const unsigned char localhostIP[4] = { 0x7F, 0x00, 0x00, 0x01 };
	std::vector<unsigned char> address = VectorUtil::MakeVector<unsigned char, 4>(localhostIP);
	const IPAddress localHostIP(EAddressFamily::IPv4, address);

	const uint16_t portNumber = connectedPeer.GetSocket().GetPort();

	const uint32_t version = P2P::PROTOCOL_VERSION;
	const Capabilities capabilities(Capabilities::FAST_SYNC_NODE); // LIGHT_CLIENT: Read P2P Config once light-clients are supported
	const uint64_t nonce = NONCE;
	Hash hash = m_config.GetEnvironment().GetGenesisHash();
	const uint64_t totalDifficulty = m_blockChainServer.GetTotalDifficulty(EChainType::CONFIRMED);
	SocketAddress senderAddress(localHostIP, m_config.GetEnvironment().GetP2PPort());
	SocketAddress receiverAddress(localHostIP, portNumber);
	const std::string& userAgent = P2P::USER_AGENT;
	const HandMessage handMessage(version, capabilities, nonce, std::move(hash), totalDifficulty, std::move(senderAddress), std::move(receiverAddress), userAgent);

	return MessageSender(m_config).Send(connectedPeer, handMessage);
}

bool ConnectionFactory::TransmitShakeMessage(ConnectedPeer& connectedPeer) const
{
	const uint16_t portNumber = connectedPeer.GetSocket().GetPort();

	const uint32_t version = P2P::PROTOCOL_VERSION;
	const Capabilities capabilities(Capabilities::FAST_SYNC_NODE); // LIGHT_CLIENT: Read P2P Config once light-clients are supported
	const uint64_t nonce = rand();
	Hash hash = m_config.GetEnvironment().GetGenesisHash();
	const uint64_t totalDifficulty = m_blockChainServer.GetTotalDifficulty(EChainType::CONFIRMED);
	const std::string& userAgent = P2P::USER_AGENT;
	const ShakeMessage shakeMessage(version, capabilities, std::move(hash), totalDifficulty, userAgent);

	return MessageSender(m_config).Send(connectedPeer, shakeMessage);
}