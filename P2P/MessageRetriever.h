#pragma once

#include "Messages/RawMessage.h"

#include <Config/Config.h>
#include <memory>
#include <vector>
#include <WinSock2.h>
#include <string>

// Forward Declarations
class ConnectedPeer;

class MessageRetriever
{
public:
	MessageRetriever(const Config& config);

	enum ERetrievalMode
	{
		BLOCKING,
		NON_BLOCKING
	};

	std::unique_ptr<RawMessage> RetrieveMessage(const ConnectedPeer& connectedPeer, const ERetrievalMode retrievalMode) const;

private:
	bool RetrievePayload(const SOCKET socket, std::vector<unsigned char>& payload) const;
	bool HasMessageBeenReceived(const SOCKET socket) const;

	void LogError() const;

	const Config& m_config;
};