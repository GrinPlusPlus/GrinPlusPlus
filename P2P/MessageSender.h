#pragma once

#include "ConnectedPeer.h"
#include "Messages/Message.h"

class MessageSender
{
public:
	static bool Send(ConnectedPeer& connectedPeer, const IMessage& message);
};