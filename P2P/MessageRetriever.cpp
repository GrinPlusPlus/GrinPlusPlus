#include "MessageRetriever.h"

#include "ConnectedPeer.h"
#include "Messages/MessageHeader.h"

#include <iostream>
#include <Serialization/ByteBuffer.h>
#include <Serialization/Serializer.h>
#include <Infrastructure/Logger.h>

MessageRetriever::MessageRetriever(const Config& config)
	: m_config(config)
{

}

std::unique_ptr<RawMessage> MessageRetriever::RetrieveMessage(const ConnectedPeer& connectedPeer, const ERetrievalMode retrievalMode) const
{
	SOCKET socket = connectedPeer.GetConnection();
	if (retrievalMode == BLOCKING || HasMessageBeenReceived(socket))
	{
		DWORD timeout = 3 * 1000;
		setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

		std::vector<unsigned char> headerBuffer(11, 0);
		int bytesReceived = recv(socket, (char*)&headerBuffer[0], 11, 0);

		if (bytesReceived == 11)
		{
			ByteBuffer byteBuffer(headerBuffer);
			MessageHeader messageHeader = MessageHeader::Deserialize(byteBuffer);

			if (!messageHeader.IsValid(m_config))
			{
				LoggerAPI::LogError(connectedPeer.GetPeer().GetIPAddress().Format() + "> FAILURE");
			}
			else
			{
				std::vector<unsigned char> payload(messageHeader.GetMessageLength());
				const bool bPayloadRetrieved = RetrievePayload(socket, payload);

				if (bPayloadRetrieved)
				{
					connectedPeer.GetPeer().UpdateLastContactTime();

					return std::make_unique<RawMessage>(std::move(RawMessage(std::move(messageHeader), payload)));
				}
			}
		}
		else
		{
			LogError();
		}
	}

	return std::unique_ptr<RawMessage>(nullptr);
}

bool MessageRetriever::RetrievePayload(const SOCKET socket, std::vector<unsigned char>& payload) const
{
	const size_t payloadSize = payload.size();
	size_t bytesReceived = 0;

	while (bytesReceived < payloadSize)
	{
		const int newBytesReceived = recv(socket, (char*)&payload[bytesReceived], (int)(payloadSize - bytesReceived), 0);
		if (newBytesReceived <= 0)
		{
			// TODO: Unexpected end of message. Throw exception?
			return false;
		}

		bytesReceived += newBytesReceived;
	}

	return true;
}

bool MessageRetriever::HasMessageBeenReceived(const SOCKET socket) const
{
	fd_set readFDS;
	readFDS.fd_count = 1;
	readFDS.fd_array[0] = socket;
	timeval timeout;
	timeout.tv_usec = 10 * 1000; // 10ms

	if (select(0, &readFDS, nullptr, nullptr, &timeout) > 0)
	{
		return true;
	}

	return false;
}

void MessageRetriever::LogError() const
{
	const int lastError = WSAGetLastError();

	TCHAR* s = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&s, 0, NULL);

	const std::string errorMessage = s;
	LoggerAPI::LogTrace(errorMessage);

	LocalFree(s);
}