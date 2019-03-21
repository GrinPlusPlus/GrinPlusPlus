#pragma once

#include "Messages/RawMessage.h"

#include <Config/Config.h>
#include <memory>
#include <vector>
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
	const Config& m_config;
};