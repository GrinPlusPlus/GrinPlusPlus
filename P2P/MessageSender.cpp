#include "MessageSender.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>

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

	const std::vector<unsigned char>& serializedMessage = serializer.GetBytes();
	const int nSendBytes = send(connectedPeer.GetSocket(), (const char*)&serializedMessage[0], (int)serializedMessage.size(), 0);
	// TODO: Handle when nSendBytes < serializedMessage.size()

	// TODO: Update stats.

	return nSendBytes != SOCKET_ERROR;
}