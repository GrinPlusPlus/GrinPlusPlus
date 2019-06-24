#include "MessageRetriever.h"
#include "ConnectionManager.h"
#include "Socket.h"
#include "Messages/MessageHeader.h"

#include <iostream>
#include <P2P/ConnectedPeer.h>
#include <P2P/SocketException.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Infrastructure/Logger.h>
#include <Common/Util/ThreadUtil.h>

MessageRetriever::MessageRetriever(const Config& config, const ConnectionManager& connectionManager)
	: m_config(config), m_connectionManager(connectionManager)
{

}

std::unique_ptr<RawMessage> MessageRetriever::RetrieveMessage(Socket& socket, const ConnectedPeer& connectedPeer, const ERetrievalMode retrievalMode) const
{
	bool hasReceivedData = socket.HasReceivedData();
	if (retrievalMode == BLOCKING)
	{
		std::chrono::time_point timeout = std::chrono::system_clock::now() + std::chrono::seconds(8);
		while (!hasReceivedData)
		{
			if (std::chrono::system_clock::now() >= timeout || m_connectionManager.IsTerminating())
			{
				return std::unique_ptr<RawMessage>(nullptr);
			}

			ThreadUtil::SleepFor(std::chrono::milliseconds(5), false);
			hasReceivedData = socket.HasReceivedData();
		}
	}

	if (hasReceivedData)
	{
		//LoggerAPI::LogTrace("MessageRetriever::RetrieveMessage - Received data from: " + connectedPeer.GetPeer().GetIPAddress().Format());
		socket.SetReceiveTimeout(5 * 1000);
		//socket.SetBlocking(true);

		std::vector<unsigned char> headerBuffer(11, 0);
		const bool received = socket.Receive(11, headerBuffer);
		if (received)
		{
			ByteBuffer byteBuffer(headerBuffer);
			MessageHeader messageHeader = MessageHeader::Deserialize(byteBuffer);

			if (!messageHeader.IsValid(m_config))
			{
				throw DeserializationException();
			}
			else
			{
				if (messageHeader.GetMessageType() != MessageTypes::Ping && messageHeader.GetMessageType() != MessageTypes::Pong)
				{
					LoggerAPI::LogTrace("MessageRetriever::RetrieveMessage - Retrieved message " + MessageTypes::ToString(messageHeader.GetMessageType()) + " from " + connectedPeer.GetPeer().GetIPAddress().Format());
				}

				std::vector<unsigned char> payload(messageHeader.GetMessageLength());
				const bool bPayloadRetrieved = socket.Receive(messageHeader.GetMessageLength(), payload);
				if (bPayloadRetrieved)
				{
					connectedPeer.GetPeer().UpdateLastContactTime();
					return std::make_unique<RawMessage>(std::move(RawMessage(std::move(messageHeader), payload)));
				}
				else
				{
					throw DeserializationException();
				}
			}
		}
		else
		{
			LoggerAPI::LogTrace("MessageRetriever::RetrieveMessage - Failed to receive message from: " + connectedPeer.GetPeer().GetIPAddress().Format());
		}
	}

	return std::unique_ptr<RawMessage>(nullptr);
}

/*void MessageRetriever::LogError() const
{
	const int lastError = WSAGetLastError();

	TCHAR* s = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&s, 0, NULL);

	const std::string errorMessage = s;
	LoggerAPI::LogTrace(errorMessage);

	LocalFree(s);
}*/