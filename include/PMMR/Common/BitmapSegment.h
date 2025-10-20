#pragma once

#include <PMMR/Common/SegmentId.h>
#include <PMMR/Common/SegmentProof.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Traits/Serializable.h>
#include <Core/Exceptions/DeserializationException.h>
#include <Common/Util/StringUtil.h>

#include <cstdint>
#include <vector>
#include <utility>

class BitmapSegmentBlock : public Traits::ISerializable
{
public:
    enum class ESerializationMode : uint8_t
    {
        Raw = 0,
        SetPositions = 1,
        UnsetPositions = 2
    };

    BitmapSegmentBlock() = default;
    BitmapSegmentBlock(
        uint8_t chunkCount,
        ESerializationMode mode,
        std::vector<uint16_t> positions,
        std::vector<uint8_t> bitmapBytes)
        : m_chunkCount(chunkCount),
        m_mode(mode),
        m_positions(std::move(positions)),
        m_bitmapBytes(std::move(bitmapBytes))
    {
    }

    BitmapSegmentBlock(const BitmapSegmentBlock& other) = default;
    BitmapSegmentBlock(BitmapSegmentBlock&& other) noexcept = default;
    virtual ~BitmapSegmentBlock() = default;

    BitmapSegmentBlock& operator=(const BitmapSegmentBlock& other) = default;
    BitmapSegmentBlock& operator=(BitmapSegmentBlock&& other) noexcept = default;

    uint8_t GetChunkCount() const noexcept { return m_chunkCount; }
    ESerializationMode GetMode() const noexcept { return m_mode; }
    const std::vector<uint16_t>& GetPositions() const noexcept { return m_positions; }
    const std::vector<uint8_t>& GetBitmapBytes() const noexcept { return m_bitmapBytes; }

    void Serialize(Serializer& serializer) const final
    {
        serializer.Append<uint8_t>(m_chunkCount);
        serializer.Append<uint8_t>(static_cast<uint8_t>(m_mode));

        switch (m_mode)
        {
        case ESerializationMode::Raw:
        {
            serializer.AppendByteVector(m_bitmapBytes);
            break;
        }
        case ESerializationMode::SetPositions:
        case ESerializationMode::UnsetPositions:
        {
            serializer.Append<uint16_t>(static_cast<uint16_t>(m_positions.size()));
            for (uint16_t position : m_positions)
            {
                serializer.Append<uint16_t>(position);
            }
            break;
        }
        default:
        {
            throw DESERIALIZATION_EXCEPTION_F(
                "Invalid bitmap serialization mode: {}",
                static_cast<uint8_t>(m_mode));
        }
        }
    }

    static BitmapSegmentBlock Deserialize(ByteBuffer& byteBuffer)
    {
        const uint8_t chunkCount = byteBuffer.Read<uint8_t>();
        const uint8_t modeValue = byteBuffer.Read<uint8_t>();

        if (modeValue > static_cast<uint8_t>(ESerializationMode::UnsetPositions)) {
            throw DESERIALIZATION_EXCEPTION_F("Invalid bitmap serialization mode: {}", modeValue);
        }

        const ESerializationMode mode = static_cast<ESerializationMode>(modeValue);

        switch (mode)
        {
            case ESerializationMode::Raw:
            {
                const size_t expectedSize = static_cast<size_t>(chunkCount) * 128;
                std::vector<uint8_t> bitmapBytes;
                bitmapBytes.reserve(expectedSize);
                for (size_t i = 0; i < expectedSize; ++i) {
                    bitmapBytes.push_back(byteBuffer.Read<uint8_t>());
                }

                return BitmapSegmentBlock(chunkCount, mode, {}, std::move(bitmapBytes));
            }
            case ESerializationMode::SetPositions:
            case ESerializationMode::UnsetPositions:
            {
                const uint16_t count = byteBuffer.Read<uint16_t>();
                std::vector<uint16_t> positions;
                positions.reserve(count);
                for (uint16_t i = 0; i < count; ++i) {
                    positions.push_back(byteBuffer.Read<uint16_t>());
                }

                return BitmapSegmentBlock(chunkCount, mode, std::move(positions), {});
            }
            default:
            {
                throw DESERIALIZATION_EXCEPTION("Invalid bitmap serialization mode");
            }
        }
    }

private:
    uint8_t m_chunkCount{ 0 };
    ESerializationMode m_mode{ ESerializationMode::Raw };
    std::vector<uint16_t> m_positions;
    std::vector<uint8_t> m_bitmapBytes;
};

class BitmapSegment : public Traits::ISerializable
{
public:
    BitmapSegment() = default;
    BitmapSegment(
        SegmentIdentifier identifier,
        std::vector<BitmapSegmentBlock> blocks,
        SegmentProof proof)
        : m_identifier(std::move(identifier)),
        m_blocks(std::move(blocks)),
        m_proof(std::move(proof))
    {
    }
    BitmapSegment(const BitmapSegment& other) = default;
    BitmapSegment(BitmapSegment&& other) noexcept = default;
    virtual ~BitmapSegment() = default;

    BitmapSegment& operator=(const BitmapSegment& other) = default;
    BitmapSegment& operator=(BitmapSegment&& other) noexcept = default;

    const SegmentIdentifier& GetIdentifier() const noexcept { return m_identifier; }
    const std::vector<BitmapSegmentBlock>& GetBlocks() const noexcept { return m_blocks; }
    const SegmentProof& GetProof() const noexcept { return m_proof; }

    void Serialize(Serializer& serializer) const final
    {
        serializer.Append(m_identifier);
        serializer.Append<uint16_t>(static_cast<uint16_t>(m_blocks.size()));
        for (const BitmapSegmentBlock& block : m_blocks) {
            block.Serialize(serializer);
        }
        m_proof.Serialize(serializer);
    }

    static BitmapSegment Deserialize(ByteBuffer& byteBuffer)
    {
        SegmentIdentifier identifier = SegmentIdentifier::Deserialize(byteBuffer);
        const uint16_t numBlocks = byteBuffer.Read<uint16_t>();

        std::vector<BitmapSegmentBlock> blocks;
        blocks.reserve(numBlocks);
        for (uint16_t i = 0; i < numBlocks; ++i) {
            blocks.emplace_back(BitmapSegmentBlock::Deserialize(byteBuffer));
        }

        SegmentProof proof = SegmentProof::Deserialize(byteBuffer);

        return BitmapSegment(
            std::move(identifier),
            std::move(blocks),
            std::move(proof));
    }

private:
    SegmentIdentifier m_identifier;
    std::vector<BitmapSegmentBlock> m_blocks;
    SegmentProof m_proof;
};