#pragma once

#include "Messages/RawMessage.h"

#include <Net/Socket.h>
#include <Core/Config.h>
#include <memory>
#include <vector>
#include <string>

// Forward Declarations
class Peer;

class MessageRetriever
{
public:
	MessageRetriever(const Config& config) : m_config(config) { }

	std::unique_ptr<RawMessage> RetrieveMessage(
		Socket& socket,
		const Peer& peer,
		const Socket::ERetrievalMode mode
	) const;

private:
	const Config& m_config;
};