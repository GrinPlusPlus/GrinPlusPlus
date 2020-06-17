#pragma once

#include "Messages/Message.h"

#include <Net/Socket.h>
#include <Config/Config.h>

class MessageSender
{
public:
	MessageSender(const Config& config);

	bool Send(Socket& socket, const IMessage& message, const EProtocolVersion protocolVersion) const;

private:
	const Config& m_config;
};