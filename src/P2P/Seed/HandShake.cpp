#include "HandShake.h"

#include "../Messages/HandMessage.h"
#include "../Messages/ShakeMessage.h"
#include "../Messages/BanReasonMessage.h"
#include "../MessageRetriever.h"
#include "../ConnectionManager.h"

#include <Net/SocketException.h>
#include <Crypto/CSPRNG.h>
#include <Common/Logger.h>
#include <Common/Util/VectorUtil.h>

static const uint64_t NONCE = CSPRNG::GenerateRandom(0, UINT64_MAX);

bool HandShake::PerformHandshake(Socket& socket, ConnectedPeer& connectedPeer, const EDirection direction) const
{
	try
	{
		LOG_TRACE_F("Performing handshake with {} - {}", socket, (direction == EDirection::INBOUND ? "inbound" : "outbound"));

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
		LOG_DEBUG_F("Failed to deserialize handshake from {}", socket);
	}
	catch (const SocketException&)
	{
		LOG_DEBUG_F("Socket exception encountered with {}", socket);
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
		std::unique_ptr<RawMessage> pReceivedMessage = MessageRetriever(m_config).RetrieveMessage(socket, *connectedPeer.GetPeer(), Socket::BLOCKING);

		if (pReceivedMessage.get() != nullptr)
		{
			const MessageTypes::EMessageType messageType = pReceivedMessage->GetMessageHeader().GetMessageType();
			if (messageType == MessageTypes::Shake)
			{
				ByteBuffer byteBuffer(pReceivedMessage->GetPayload());
				const ShakeMessage shakeMessage = ShakeMessage::Deserialize(byteBuffer);

				const uint32_t version = (std::min)(P2P::PROTOCOL_VERSION, shakeMessage.GetVersion());
				connectedPeer.UpdateVersion(version);
				connectedPeer.UpdateCapabilities(shakeMessage.GetCapabilities());
				connectedPeer.UpdateUserAgent(shakeMessage.GetUserAgent());
				connectedPeer.UpdateTotals(shakeMessage.GetTotalDifficulty(), 0);

				return true;
			}
			else if (messageType == MessageTypes::BanReasonMsg)
			{
				ByteBuffer byteBuffer(pReceivedMessage->GetPayload());
				const BanReasonMessage banReasonMessage = BanReasonMessage::Deserialize(byteBuffer);

				LOG_DEBUG_F("Ban message received from {} with reason {}", socket, std::to_string(banReasonMessage.GetBanReason()));
				return false;
			}
			else
			{
				LOG_DEBUG_F("Expected shake from {} but received {}.", socket, MessageTypes::ToString(messageType));
				return false;
			}
		}

		LOG_TRACE_F("Shake message not received from {}", socket);
		return false;
	}

	LOG_DEBUG_F("Hand message not sent to {}", socket);
	return false;
}

bool HandShake::PerformInboundHandshake(Socket& socket, ConnectedPeer& connectedPeer) const
{
	// Get Hand Message
	std::unique_ptr<RawMessage> pReceivedMessage = MessageRetriever(m_config).RetrieveMessage(socket, *connectedPeer.GetPeer(), Socket::BLOCKING);
	if (pReceivedMessage != nullptr)
	{
		if (pReceivedMessage->GetMessageHeader().GetMessageType() == MessageTypes::Hand)
		{
			ByteBuffer byteBuffer(pReceivedMessage->GetPayload());
			const HandMessage handMessage = HandMessage::Deserialize(byteBuffer);

			if (handMessage.GetNonce() != NONCE && !m_connectionManager.IsConnected(connectedPeer.GetPeer()->GetIPAddress()))
			{
				connectedPeer.UpdateCapabilities(handMessage.GetCapabilities());
				connectedPeer.UpdateUserAgent(handMessage.GetUserAgent());
				connectedPeer.UpdateTotals(handMessage.GetTotalDifficulty(), 0);

				const uint32_t version = (std::min)(P2P::PROTOCOL_VERSION, handMessage.GetVersion());
				connectedPeer.UpdateVersion(version);

				// Send Shake Message
				if (TransmitShakeMessage(socket, version))
				{
					return true;
				}
				else
				{
					LOG_DEBUG_F("Failed to transmit shake message to {}", socket);
					return false;
				}
			}
			else if (handMessage.GetNonce() == NONCE)
			{
				LOG_DEBUG_F("Connected to self {}. Nonce: {}", socket, NONCE);
			}
			else
			{
				LOG_DEBUG_F("Already connected to {}", connectedPeer);
			}
		}
		else
		{
			LOG_DEBUG_F("First message from {} was of type {}", socket, MessageTypes::ToString(pReceivedMessage->GetMessageHeader().GetMessageType()));
			return false;
		}
	}

	LOG_TRACE_F("Unable to connect to {}.", socket);
	return false;
}

bool HandShake::TransmitHandMessage(Socket& socket) const
{
	const IPAddress localHostIP = IPAddress::CreateV4({ 0x7F, 0x00, 0x00, 0x01 });

	const uint16_t portNumber = socket.GetPort();

	const uint32_t version = P2P::PROTOCOL_VERSION;
	const Capabilities capabilities(Capabilities::FAST_SYNC_NODE); // LIGHT_CLIENT: Read P2P Config once light-clients are supported
	const uint64_t nonce = NONCE;
	Hash hash = m_config.GetEnvironment().GetGenesisHash();
	const uint64_t totalDifficulty = m_pSyncStatus->GetBlockDifficulty();
	SocketAddress senderAddress(localHostIP, m_config.GetEnvironment().GetP2PPort());
	SocketAddress receiverAddress(localHostIP, portNumber);
	const std::string& userAgent = P2P::USER_AGENT;
	const HandMessage handMessage(version, capabilities, nonce, std::move(hash), totalDifficulty, std::move(senderAddress), std::move(receiverAddress), userAgent);

	std::vector<uint8_t> serialized_message = handMessage.Serialize(
		m_config.GetEnvironment(),
		ProtocolVersion::Local()
	);

	return socket.Send(serialized_message, true);
}

bool HandShake::TransmitShakeMessage(Socket& socket, const uint32_t protocolVersion) const
{
	const Capabilities capabilities(Capabilities::FAST_SYNC_NODE); // LIGHT_CLIENT: Read P2P Config once light-clients are supported
	Hash hash = m_config.GetEnvironment().GetGenesisHash();
	const uint64_t totalDifficulty = m_pSyncStatus->GetBlockDifficulty();
	const std::string& userAgent = P2P::USER_AGENT;
	const ShakeMessage shakeMessage(protocolVersion, capabilities, std::move(hash), totalDifficulty, userAgent);

	std::vector<uint8_t> serialized_message = shakeMessage.Serialize(
		m_config.GetEnvironment(),
		ProtocolVersion::ToEnum(protocolVersion)
	);

	return socket.Send(serialized_message, true);
}
