#pragma once

#include "Messages/RawMessage.h"

#include <Config/Config.h>
#include <memory>
#include <vector>
#include <string>

// Forward Declarations
class Socket;
class ConnectedPeer;
class ConnectionManager;

class MessageRetriever
{
public:
	MessageRetriever(const Config& config, const ConnectionManager& connectionManager);

	enum ERetrievalMode
	{
		BLOCKING,
		NON_BLOCKING
	};

	std::unique_ptr<RawMessage> RetrieveMessage(Socket& socket, const ConnectedPeer& connectedPeer, const ERetrievalMode retrievalMode) const;

private:
	const Config& m_config;
	const ConnectionManager& m_connectionManager;
};