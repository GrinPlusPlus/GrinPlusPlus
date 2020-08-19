#pragma once

#include <Core/Serialization/Serializer.h>
#include <Core/Serialization/ByteBuffer.h>
#include <cstdint>

static const uint8_t USER_METADATA_FORMAT = 0;

class UserMetadata
{
public:
	UserMetadata(const uint32_t nextTxId, const uint64_t refreshBlockHeight, const uint64_t restoreLeafIndex)
		: m_nextTxId(nextTxId), m_refreshBlockHeight(refreshBlockHeight), m_restoreLeafIndex(restoreLeafIndex) { }

	uint32_t GetNextTxId() const noexcept { return m_nextTxId; }
	uint64_t GetRefreshBlockHeight() const noexcept { return m_refreshBlockHeight; }
	uint64_t GetRestoreLeafIndex() const noexcept { return m_restoreLeafIndex; }

	void Serialize(Serializer& serializer) const
	{
		serializer.Append<uint8_t>(USER_METADATA_FORMAT);
		serializer.Append<uint32_t>(m_nextTxId);
		serializer.Append<uint64_t>(m_refreshBlockHeight);
		serializer.Append<uint64_t>(m_restoreLeafIndex);
	}

	static UserMetadata Deserialize(ByteBuffer& byteBuffer)
	{
		const uint8_t format = byteBuffer.ReadU8();
		if (format != USER_METADATA_FORMAT)
		{
			throw DESERIALIZATION_EXCEPTION_F("Expected format {}, but was {}", USER_METADATA_FORMAT, format);
		}

		const uint32_t nextTxId = byteBuffer.ReadU32();
		const uint64_t refreshBlockHeight = byteBuffer.ReadU64();
		const uint64_t restoreLeafIndex = byteBuffer.ReadU64();
		return UserMetadata(nextTxId, refreshBlockHeight, restoreLeafIndex);
	}

private:
	uint32_t m_nextTxId;
	uint64_t m_refreshBlockHeight;
	uint64_t m_restoreLeafIndex;
};