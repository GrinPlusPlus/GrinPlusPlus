#pragma once

#include "Message.h"

#include <Crypto/Models/Hash.h>
#include <Core/Models/TransactionKernel.h>
#include <Core/Models/OutputIdentifier.h>
#include <Crypto/Models/RangeProof.h>
#include <PMMR/Common/Segment.h>
#include <PMMR/Common/BitmapSegment.h>

#include <utility>

static constexpr size_t PIBD_OUTPUT_DATA_SIZE = 34;
static constexpr size_t PIBD_RANGE_PROOF_DATA_SIZE = 683;
static constexpr size_t PIBD_KERNEL_DATA_SIZE = 114;

template<MessageTypes::EMessageType MessageType, size_t DATA_SIZE, class DATA_TYPE>
class SegmentResponseMessage : public IMessage
{
public:
    using SegmentType = Segment<DATA_SIZE, DATA_TYPE>;
    using Self = SegmentResponseMessage<MessageType, DATA_SIZE, DATA_TYPE>;

    SegmentResponseMessage(Hash blockHash, SegmentType segment)
        : m_blockHash(std::move(blockHash)), m_segment(std::move(segment)) { }
    SegmentResponseMessage(const SegmentResponseMessage& other) = default;
    SegmentResponseMessage(SegmentResponseMessage&& other) noexcept = default;
    virtual ~SegmentResponseMessage() = default;

    SegmentResponseMessage& operator=(const SegmentResponseMessage& other) = default;
    SegmentResponseMessage& operator=(SegmentResponseMessage&& other) noexcept = default;

    IMessagePtr Clone() const final { return IMessagePtr(new SegmentResponseMessage(*this)); }

    MessageTypes::EMessageType GetMessageType() const final { return MessageType; }
    const Hash& GetBlockHash() const noexcept { return m_blockHash; }
    const SegmentType& GetSegment() const noexcept { return m_segment; }

    static Self Deserialize(ByteBuffer& byteBuffer)
    {
        Hash blockHash = byteBuffer.ReadBigInteger<32>();
        SegmentType segment = SegmentType::Deserialize(byteBuffer);

        return Self(std::move(blockHash), std::move(segment));
    }

protected:
    void SerializeBody(Serializer& serializer) const final
    {
        serializer.AppendBigInteger<32>(m_blockHash);
        m_segment.Serialize(serializer);
    }

private:
    Hash m_blockHash;
    SegmentType m_segment;
};

class OutputSegmentMessage : public IMessage
{
public:
    using SegmentType = Segment<PIBD_OUTPUT_DATA_SIZE, OutputIdentifier>;

    OutputSegmentMessage(Hash blockHash, SegmentType segment, Hash bitmapRoot)
        : m_blockHash(std::move(blockHash)),
        m_segment(std::move(segment)),
        m_outputBitmapRoot(std::move(bitmapRoot))
    {
    }
    OutputSegmentMessage(const OutputSegmentMessage& other) = default;
    OutputSegmentMessage(OutputSegmentMessage&& other) noexcept = default;
    virtual ~OutputSegmentMessage() = default;

    OutputSegmentMessage& operator=(const OutputSegmentMessage& other) = default;
    OutputSegmentMessage& operator=(OutputSegmentMessage&& other) noexcept = default;

    IMessagePtr Clone() const final { return IMessagePtr(new OutputSegmentMessage(*this)); }

    MessageTypes::EMessageType GetMessageType() const final { return MessageTypes::OutputSegment; }
    const Hash& GetBlockHash() const noexcept { return m_blockHash; }
    const SegmentType& GetSegment() const noexcept { return m_segment; }
    const Hash& GetOutputBitmapRoot() const noexcept { return m_outputBitmapRoot; }

    static OutputSegmentMessage Deserialize(ByteBuffer& byteBuffer)
    {
        Hash blockHash = byteBuffer.ReadBigInteger<32>();
        SegmentType segment = SegmentType::Deserialize(byteBuffer);
        Hash bitmapRoot = byteBuffer.ReadBigInteger<32>();

        return OutputSegmentMessage(
            std::move(blockHash),
            std::move(segment),
            std::move(bitmapRoot));
    }

protected:
    void SerializeBody(Serializer& serializer) const final
    {
        serializer.AppendBigInteger<32>(m_blockHash);
        m_segment.Serialize(serializer);
        serializer.AppendBigInteger<32>(m_outputBitmapRoot);
    }

private:
    Hash m_blockHash;
    SegmentType m_segment;
    Hash m_outputBitmapRoot;
};

class OutputBitmapSegmentMessage : public IMessage
{
public:
    OutputBitmapSegmentMessage(Hash blockHash, BitmapSegment segment, Hash outputRoot)
        : m_blockHash(std::move(blockHash)),
        m_segment(std::move(segment)),
        m_outputRoot(std::move(outputRoot))
    {
    }
    OutputBitmapSegmentMessage(const OutputBitmapSegmentMessage& other) = default;
    OutputBitmapSegmentMessage(OutputBitmapSegmentMessage&& other) noexcept = default;
    virtual ~OutputBitmapSegmentMessage() = default;

    OutputBitmapSegmentMessage& operator=(const OutputBitmapSegmentMessage& other) = default;
    OutputBitmapSegmentMessage& operator=(OutputBitmapSegmentMessage&& other) noexcept = default;

    IMessagePtr Clone() const final { return IMessagePtr(new OutputBitmapSegmentMessage(*this)); }

    MessageTypes::EMessageType GetMessageType() const final { return MessageTypes::OutputBitmapSegment; }
    const Hash& GetBlockHash() const noexcept { return m_blockHash; }
    const BitmapSegment& GetSegment() const noexcept { return m_segment; }
    const Hash& GetOutputRoot() const noexcept { return m_outputRoot; }

    static OutputBitmapSegmentMessage Deserialize(ByteBuffer& byteBuffer)
    {
        Hash blockHash = byteBuffer.ReadBigInteger<32>();
        BitmapSegment segment = BitmapSegment::Deserialize(byteBuffer);
        Hash outputRoot = byteBuffer.ReadBigInteger<32>();

        return OutputBitmapSegmentMessage(
            std::move(blockHash),
            std::move(segment),
            std::move(outputRoot));
    }

protected:
    void SerializeBody(Serializer& serializer) const final
    {
        serializer.AppendBigInteger<32>(m_blockHash);
        m_segment.Serialize(serializer);
        serializer.AppendBigInteger<32>(m_outputRoot);
    }

private:
    Hash m_blockHash;
    BitmapSegment m_segment;
    Hash m_outputRoot;
};

using RangeProofSegmentMessage = SegmentResponseMessage<MessageTypes::RangeProofSegment, PIBD_RANGE_PROOF_DATA_SIZE, RangeProof>;
using KernelSegmentMessage = SegmentResponseMessage<MessageTypes::KernelSegment, PIBD_KERNEL_DATA_SIZE, TransactionKernel>;