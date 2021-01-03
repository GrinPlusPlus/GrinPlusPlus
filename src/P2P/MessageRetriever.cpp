#include "MessageRetriever.h"
#include "Messages/MessageHeader.h"

#include <P2P/Peer.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Common/Logger.h>

std::unique_ptr<RawMessage> MessageRetriever::RetrieveMessage(
	Socket& socket,
	const Peer& peer,
	const Socket::ERetrievalMode mode) const
{
	std::vector<uint8_t> headerBuffer;
	const bool received = socket.Receive(11, true, mode, headerBuffer);
	if (!received) {
		return std::unique_ptr<RawMessage>(nullptr);
	}

	ByteBuffer byteBuffer(std::move(headerBuffer));
	MessageHeader messageHeader = MessageHeader::Deserialize(byteBuffer);

	const auto type = messageHeader.GetMessageType();
	if (type != MessageTypes::Ping && type != MessageTypes::Pong) {
		LOG_TRACE_F( "Received '{}' message from {}", MessageTypes::ToString(type), peer);
	}

	std::vector<uint8_t> payload;
	const bool bPayloadRetrieved = socket.Receive(
		messageHeader.GetMessageLength(),
		false,
		Socket::BLOCKING,
		payload
	);
	if (!bPayloadRetrieved) {
		throw DESERIALIZATION_EXCEPTION("Expected payload not received");
	}

	peer.UpdateLastContactTime();
	return std::make_unique<RawMessage>(std::move(messageHeader), std::move(payload));
}