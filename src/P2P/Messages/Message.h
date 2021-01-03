#pragma once

#include "MessageTypes.h"

#include <P2P/Common.h>
#include <Core/Global.h>
#include <Core/Config.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

class IMessage
{
public:
	virtual ~IMessage() = default;
	virtual std::shared_ptr<IMessage> Clone() const = 0;

	virtual MessageTypes::EMessageType GetMessageType() const = 0;
	virtual void SerializeBody(Serializer& serializer) const = 0;

	std::vector<uint8_t> Serialize(const EProtocolVersion protocolVersion) const
	{
		Serializer serializer(protocolVersion);
		serializer.AppendByteVector(Global::GetConfig().GetMagicBytes());
		serializer.Append<uint8_t>((uint8_t)GetMessageType());

		Serializer bodySerializer(protocolVersion);
		SerializeBody(bodySerializer);
		serializer.Append<uint64_t>(bodySerializer.size());
		serializer.AppendByteVector(bodySerializer.GetBytes());

		return serializer.GetBytes();
	}
};

typedef std::shared_ptr<IMessage> IMessagePtr;