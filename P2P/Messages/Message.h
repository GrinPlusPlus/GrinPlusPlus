#pragma once

#include "../Common.h"
#include "MessageTypes.h"

#include <Serialization/ByteBuffer.h>
#include <Serialization/Serializer.h>

class IMessage
{
public:
	virtual ~IMessage() = default;
	virtual IMessage* Clone() const = 0;

	virtual MessageTypes::EMessageType GetMessageType() const = 0;

	void Serialize(Serializer& serializer) const
	{
		serializer.AppendByteVector(P2P::MAGIC_BYTES);
		serializer.Append<uint8_t>((uint8_t)GetMessageType());

		Serializer bodySerializer;
		SerializeBody(bodySerializer);

		const uint64_t messageLength = bodySerializer.GetBytes().size();
		serializer.Append<uint64_t>(messageLength);

		serializer.AppendByteVector(bodySerializer.GetBytes());
	}

protected:
	virtual void SerializeBody(Serializer& serializer) const = 0;
};