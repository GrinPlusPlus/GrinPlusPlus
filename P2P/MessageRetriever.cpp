#include "MessageRetriever.h"
#include "ConnectionManager.h"

#include "Messages/MessageHeader.h"

#include <iostream>
#include <P2P/ConnectedPeer.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Infrastructure/Logger.h>
#include <Net/SocketException.h>

MessageRetriever::MessageRetriever(const Config& config, const ConnectionManager& connectionManager)
	: m_config(config), m_connectionManager(connectionManager)
{

}

std::unique_ptr<RawMessage> MessageRetriever::RetrieveMessage(const ConnectedPeer& connectedPeer, const ERetrievalMode retrievalMode) const
{
	Socket& socket = connectedPeer.GetSocket();

	try
	{
		bool hasReceivedData = socket.HasReceivedData(10);
		if (retrievalMode == BLOCKING)
		{
			std::chrono::time_point timeout = std::chrono::system_clock::now() + std::chrono::seconds(8);
			while (!hasReceivedData)
			{
				if (std::chrono::system_clock::now() >= timeout || m_connectionManager.IsTerminating())
				{
					return std::unique_ptr<RawMessage>(nullptr);
				}

				hasReceivedData = socket.HasReceivedData(10);
			}
		}

		if (hasReceivedData)
		{
			socket.SetReceiveTimeout(5 * 1000);

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
						LoggerAPI::LogTrace("Retrieved message " + MessageTypes::ToString(messageHeader.GetMessageType()) + " from " + connectedPeer.GetPeer().GetIPAddress().Format());
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
		}
	}
	catch (const SocketException&)
	{
		LoggerAPI::LogDebug("MessageRetriever::RetrieveMessage - Socket exception occurred on socket " + socket.GetSocketAddress().Format());
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