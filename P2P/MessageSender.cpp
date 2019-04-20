#include "MessageSender.h"

#include <Infrastructure/Logger.h>

MessageSender::MessageSender(const Config& config)
	: m_config(config)
{

}

bool MessageSender::Send(ConnectedPeer& connectedPeer, const IMessage& message) const
{
	Serializer serializer;
	serializer.AppendByteVector(m_config.GetEnvironment().GetMagicBytes());
	serializer.Append<uint8_t>((uint8_t)message.GetMessageType());

	Serializer bodySerializer;
	message.SerializeBody(bodySerializer);
	serializer.Append<uint64_t>(bodySerializer.GetBytes().size());
	serializer.AppendByteVector(bodySerializer.GetBytes());

	if (message.GetMessageType() != MessageTypes::Ping && message.GetMessageType() != MessageTypes::Pong)
	{
		LoggerAPI::LogTrace("Sending message " + MessageTypes::ToString(message.GetMessageType()) + " to " + connectedPeer.GetPeer().GetIPAddress().Format());
	}

	return connectedPeer.GetSocket().Send(serializer.GetBytes());
}