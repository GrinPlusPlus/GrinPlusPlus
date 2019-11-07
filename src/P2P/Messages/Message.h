#pragma once

#include "MessageTypes.h"

#include <P2P/Common.h>
#include <Config/Config.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

class IMessage
{
public:
	virtual ~IMessage() = default;
	virtual std::shared_ptr<IMessage> Clone() const = 0;

	virtual MessageTypes::EMessageType GetMessageType() const = 0;
	virtual void SerializeBody(Serializer& serializer) const = 0;
};

typedef std::shared_ptr<IMessage> IMessagePtr;