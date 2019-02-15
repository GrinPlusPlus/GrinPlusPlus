#pragma once

#include "Messages/Message.h"

#include <P2P/ConnectedPeer.h>
#include <Config/Config.h>

class MessageSender
{
public:
	MessageSender(const Config& config);

	bool Send(ConnectedPeer& connectedPeer, const IMessage& message) const;

private:
	const Config& m_config;
};