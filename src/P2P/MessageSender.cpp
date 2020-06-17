#include "MessageSender.h"

#include <Infrastructure/Logger.h>

MessageSender::MessageSender(const Config& config)
	: m_config(config)
{

}

bool MessageSender::Send(Socket& socket, const IMessage& message, const EProtocolVersion protocolVersion) const
{
	Serializer serializer(protocolVersion);
	serializer.AppendByteVector(m_config.GetEnvironment().GetMagicBytes());
	serializer.Append<uint8_t>((uint8_t)message.GetMessageType());

	Serializer bodySerializer;
	message.SerializeBody(bodySerializer);
	serializer.Append<uint64_t>(bodySerializer.size());
	serializer.AppendByteVector(bodySerializer.GetBytes());

	if (message.GetMessageType() != MessageTypes::Ping && message.GetMessageType() != MessageTypes::Pong)
	{
		LOG_TRACE_F("Sending message ({}) to ({})", MessageTypes::ToString(message.GetMessageType()), socket);
	}

	return socket.Send(serializer.GetBytes(), true);
}