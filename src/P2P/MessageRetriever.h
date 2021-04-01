#pragma once

#include "Messages/RawMessage.h"

#include <Net/Socket.h>
#include <memory>
#include <vector>
#include <string>

// Forward Declarations
class ConnectedPeer;
class Peer;

class MessageRetriever
{
public:
	static std::unique_ptr<RawMessage> RetrieveMessage(
		Socket& socket,
		const ConnectedPeer& connected_peer,
		const Socket::ERetrievalMode mode
	);

	static std::unique_ptr<RawMessage> RetrieveMessage(
		Socket& socket,
		const Peer& peer,
		const Socket::ERetrievalMode mode
	);
};