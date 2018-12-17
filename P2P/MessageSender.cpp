#include "MessageSender.h"

#include <Infrastructure/Logger.h>
#include <HexUtil.h>

bool MessageSender::Send(ConnectedPeer& connectedPeer, const IMessage& message)
{
	Serializer serializer;
	message.Serialize(serializer);
	const std::vector<unsigned char>& serializedMessage = serializer.GetBytes();

	const std::string hexMessage = HexUtil::ConvertToHex(serializedMessage, true, true);
	//LoggerAPI::LogInfo(connectedPeer.GetPeer().GetIPAddress().Format() + "> Sent(" + std::to_string(message.GetMessageType()) + "): " + hexMessage);

	const int nSendBytes = send(connectedPeer.GetConnection(), (const char*)&serializedMessage[0], (int)serializedMessage.size(), 0);

	// TODO: Update stats.

	return nSendBytes != SOCKET_ERROR;
}