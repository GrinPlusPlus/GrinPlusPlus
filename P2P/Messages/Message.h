#pragma once

#include "MessageTypes.h"

#include <P2P/Common.h>
#include <Config/Config.h>
#include <Serialization/ByteBuffer.h>
#include <Serialization/Serializer.h>

class IMessage
{
public:
	virtual ~IMessage() = default;
	virtual IMessage* Clone() const = 0;

	virtual MessageTypes::EMessageType GetMessageType() const = 0;
	virtual void SerializeBody(Serializer& serializer) const = 0;
};