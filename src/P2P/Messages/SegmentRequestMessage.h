#pragma once

#include "Message.h"

#include <Crypto/Models/Hash.h>
#include <PMMR/Common/SegmentId.h>

#include <utility>

template<MessageTypes::EMessageType MessageType>
class SegmentRequestMessage : public IMessage
{
public:
    using Self = SegmentRequestMessage<MessageType>;

    SegmentRequestMessage(Hash blockHash, SegmentIdentifier identifier)
        : m_blockHash(std::move(blockHash)), m_identifier(std::move(identifier)) { }
    SegmentRequestMessage(const SegmentRequestMessage& other) = default;
    SegmentRequestMessage(SegmentRequestMessage&& other) noexcept = default;
    virtual ~SegmentRequestMessage() = default;

    SegmentRequestMessage& operator=(const SegmentRequestMessage& other) = default;
    SegmentRequestMessage& operator=(SegmentRequestMessage&& other) noexcept = default;

    IMessagePtr Clone() const final { return IMessagePtr(new SegmentRequestMessage(*this)); }

    MessageTypes::EMessageType GetMessageType() const final { return MessageType; }
    const Hash& GetBlockHash() const noexcept { return m_blockHash; }
    const SegmentIdentifier& GetIdentifier() const noexcept { return m_identifier; }

    static Self Deserialize(ByteBuffer& byteBuffer)
    {
        Hash blockHash = byteBuffer.ReadBigInteger<32>();
        SegmentIdentifier identifier = SegmentIdentifier::Deserialize(byteBuffer);

        return Self(std::move(blockHash), std::move(identifier));
    }

protected:
    void SerializeBody(Serializer& serializer) const final
    {
        serializer.AppendBigInteger<32>(m_blockHash);
        serializer.Append(m_identifier);
    }

private:
    Hash m_blockHash;
    SegmentIdentifier m_identifier;
};

using GetOutputBitmapSegmentMessage = SegmentRequestMessage<MessageTypes::GetOutputBitmapSegment>;
using GetOutputSegmentMessage = SegmentRequestMessage<MessageTypes::GetOutputSegment>;
using GetRangeProofSegmentMessage = SegmentRequestMessage<MessageTypes::GetRangeProofSegment>;
using GetKernelSegmentMessage = SegmentRequestMessage<MessageTypes::GetKernelSegment>;