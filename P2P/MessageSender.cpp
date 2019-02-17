#include "MessageSender.h"

#include <Infrastructure/Logger.h>
#include <Common/Util/HexUtil.h>

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

	const uint64_t messageLength = bodySerializer.GetBytes().size();
	serializer.Append<uint64_t>(messageLength);

	serializer.AppendByteVector(bodySerializer.GetBytes());

	const std::vector<unsigned char>& serializedMessage = serializer.GetBytes();

	const std::string hexMessage = HexUtil::ConvertToHex(serializedMessage, true, true);

	const int nSendBytes = send(connectedPeer.GetSocket(), (const char*)&serializedMessage[0], (int)serializedMessage.size(), 0);

	// TODO: Update stats.

	return nSendBytes != SOCKET_ERROR;
}