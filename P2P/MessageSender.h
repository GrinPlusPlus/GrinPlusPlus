#pragma once

#include "ConnectedPeer.h"
#include "Messages/Message.h"

#include <Config/Config.h>

class MessageSender
{
public:
	MessageSender(const Config& config);

	bool Send(ConnectedPeer& connectedPeer, const IMessage& message) const;

private:
	const Config& m_config;
};