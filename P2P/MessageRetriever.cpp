#include "MessageRetriever.h"

#include "Messages/MessageHeader.h"

#include <iostream>
#include <P2P/ConnectedPeer.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Infrastructure/Logger.h>

MessageRetriever::MessageRetriever(const Config& config)
	: m_config(config)
{

}

std::unique_ptr<RawMessage> MessageRetriever::RetrieveMessage(const ConnectedPeer& connectedPeer, const ERetrievalMode retrievalMode) const
{
	Socket& socket = connectedPeer.GetSocket();
	if (retrievalMode == BLOCKING || socket.HasReceivedData(10))
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