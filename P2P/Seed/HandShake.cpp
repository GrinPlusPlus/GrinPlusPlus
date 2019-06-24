#include "HandShake.h"

#include "../Messages/HandMessage.h"
#include "../Messages/ShakeMessage.h"
#include "../Messages/BanReasonMessage.h"
#include "../MessageRetriever.h"
#include "../MessageSender.h"
#include "../ConnectionManager.h"

#include <P2P/SocketException.h>
#include <Crypto/RandomNumberGenerator.h>
#include <Infrastructure/Logger.h>
#include <Common/Util/VectorUtil.h>

static const uint64_t NONCE = RandomNumberGenerator::GenerateRandom(0, UINT64_MAX);

HandShake::HandShake(const Config& config, ConnectionManager& connectionManager, PeerManager& peerManager, IBlockChainServer& blockChainServer)
	: m_config(config), m_connectionManager(connectionManager), m_peerManager(peerManager), m_blockChainServer(blockChainServer)
{

}

bool HandShake::PerformHandshake(Socket& socket, ConnectedPeer& connectedPeer, const EDirection direction) const
{
	try
	{
		LoggerAPI::LogTrace("HandShake::PerformHandshake - Performing handshake with " 
			+ socket.GetSocketAddress().Format() 
			+ std::string(" - direction: ")
			+ (direction == EDirection::INBOUND ? "inbound" : "outbound")
		);

		if (direction == EDirection::OUTBOUND)
		{
			return PerformOutboundHandshake(socket, connectedPeer);
		}
		else
		{
			return PerformInboundHandshake(socket, connectedPeer);
		}
	}
	catch (const DeserializationException&)
	{
		LoggerAPI::LogDebug("HandShake::PerformHandshake - Failed to deserialize handshake from " + socket.GetSocketAddress().Format());
	}
	catch (const SocketException&)
	{
		LoggerAPI::LogDebug("HandShake::PerformHandshake - Socket exception encountered with " + socket.GetSocketAddress().Format());
	}

	return false;
}

bool HandShake::PerformOutboundHandshake(Socket& socket, ConnectedPeer& connectedPeer) const
{
	// Send Hand Message
	const bool bHandMessageSent = TransmitHandMessage(socket);
	if (bHandMessageSent)
	{
		// Get Shake Message
		std::unique_ptr<RawMessage> pReceivedMessage = MessageRetriever(m_config, m_connectionManager).RetrieveMessage(socket, connectedPeer, MessageRetriever::BLOCKING);

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

				return true;
			}
			else if (messageType == MessageTypes::BanReasonMsg)
			{
				ByteBuffer byteBuffer(pReceivedMessage->GetPayload());
				const BanReasonMessage banReasonMessage = BanReasonMessage::Deserialize(byteBuffer);

				LoggerAPI::LogDebug("HandShake::PerformOutboundHandshake - Ban message received from " 
					+ socket.GetSocketAddress().Format() 
					+ " with reason: " 
					+ std::to_string(banReasonMessage.GetBanReason())
				);
				return false;
			}
			else
			{
				LoggerAPI::LogDebug("HandShake::PerformOutboundHandshake - Expected shake from " 
					+ socket.GetSocketAddress().Format() 
					+ " but received " 
					+ MessageTypes::ToString(messageType)
				);
				return false;
			}
		}

		LoggerAPI::LogTrace("HandShake::PerformOutboundHandshake - Shake message not received from " + socket.GetSocketAddress().Format());
		return false;
	}

	LoggerAPI::LogDebug("HandShake::PerformOutboundHandshake - Hand message not sent to " + socket.GetSocketAddress().Format());
	return false;
}

bool HandShake::PerformInboundHandshake(Socket & socket, ConnectedPeer & connectedPeer) const
{
	// Get Hand Message
	std::unique_ptr<RawMessage> pReceivedMessage = MessageRetriever(m_config, m_connectionManager).RetrieveMessage(socket, connectedPeer, MessageRetriever::BLOCKING);
	if (pReceivedMessage != nullptr)
	{
		if (pReceivedMessage->GetMessageHeader().GetMessageType() == MessageTypes::Hand)
		{
			ByteBuffer byteBuffer(pReceivedMessage->GetPayload());
			const HandMessage handMessage = HandMessage::Deserialize(byteBuffer);

			if (handMessage.GetNonce() != NONCE && !m_connectionManager.IsConnected(connectedPeer.GetPeer().GetIPAddress()))
			{
				connectedPeer.UpdateVersion(handMessage.GetVersion());
				connectedPeer.UpdateCapabilities(handMessage.GetCapabilities());
				connectedPeer.UpdateUserAgent(handMessage.GetUserAgent());
				connectedPeer.UpdateTotals(handMessage.GetTotalDifficulty(), 0);

				// Send Shake Message
				if (TransmitShakeMessage(socket))
				{
					return true;
				}
				else
				{
					LoggerAPI::LogDebug("HandShake::PerformInboundHandshake - Failed to transmit shake message to " + socket.GetSocketAddress().Format());
					return false;
				}
			}
			else if (handMessage.GetNonce() == NONCE)
			{
				LoggerAPI::LogDebug("HandShake::PerformInboundHandshake - Connected to self(" + socket.GetSocketAddress().Format() + "). Nonce: " + std::to_string(NONCE));
			}
			else
			{
				LoggerAPI::LogDebug("HandShake::PerformInboundHandshake - Already connected to " + connectedPeer.GetPeer().GetIPAddress().Format());
			}
		}
		else
		{
			LoggerAPI::LogDebug("HandShake::PerformInboundHandshake - First message from " + socket.GetSocketAddress().Format()
				+ " was of type " + std::to_string(pReceivedMessage->GetMessageHeader().GetMessageType()));
			return false;
		}
	}

	LoggerAPI::LogTrace("HandShake::PerformInboundHandshake - Unable to connect to " + socket.GetSocketAddress().Format());
	return false;
}

bool HandShake::TransmitHandMessage(Socket & socket) const
{
	const unsigned char localhostIP[4] = { 0x7F, 0x00, 0x00, 0x01 };
	std::vector<unsigned char> address = VectorUtil::MakeVector<unsigned char, 4>(localhostIP);
	const IPAddress localHostIP(EAddressFamily::IPv4, address);

	const uint16_t portNumber = socket.GetPort();

	const uint32_t version = P2P::PROTOCOL_VERSION;
	const Capabilities capabilities(Capabilities::FAST_SYNC_NODE); // LIGHT_CLIENT: Read P2P Config once light-clients are supported
	const uint64_t nonce = NONCE;
	Hash hash = m_config.GetEnvironment().GetGenesisHash();
	const uint64_t totalDifficulty = m_blockChainServer.GetTotalDifficulty(EChainType::CONFIRMED);
	SocketAddress senderAddress(localHostIP, m_config.GetEnvironment().GetP2PPort());
	SocketAddress receiverAddress(localHostIP, portNumber);
	const std::string& userAgent = P2P::USER_AGENT;
	const HandMessage handMessage(version, capabilities, nonce, std::move(hash), totalDifficulty, std::move(senderAddress), std::move(receiverAddress), userAgent);

	return MessageSender(m_config).Send(socket, handMessage);
}

bool HandShake::TransmitShakeMessage(Socket & socket) const
{
	const uint16_t portNumber = socket.GetPort();

	const uint32_t version = P2P::PROTOCOL_VERSION;
	const Capabilities capabilities(Capabilities::FAST_SYNC_NODE); // LIGHT_CLIENT: Read P2P Config once light-clients are supported
	const uint64_t nonce = rand();
	Hash hash = m_config.GetEnvironment().GetGenesisHash();
	const uint64_t totalDifficulty = m_blockChainServer.GetTotalDifficulty(EChainType::CONFIRMED);
	const std::string& userAgent = P2P::USER_AGENT;
	const ShakeMessage shakeMessage(version, capabilities, std::move(hash), totalDifficulty, userAgent);

	return MessageSender(m_config).Send(socket, shakeMessage);
}