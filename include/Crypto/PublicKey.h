#pragma once

#include <Crypto/BigInteger.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

class PublicKey
{
public:
	PublicKey() = default;
	PublicKey(CBigInteger<33>&& compressedKey)
		: m_compressedKey(std::move(compressedKey)) { }

	bool operator==(const PublicKey& rhs) const noexcept { return m_compressedKey == rhs.m_compressedKey; }

	const CBigInteger<33>& GetCompressedBytes() const noexcept { return m_compressedKey; }
	const std::vector<unsigned char>& GetCompressedVec() const noexcept { return m_compressedKey.GetData(); }
	std::string ToHex() const noexcept { return m_compressedKey.ToHex(); }

	const unsigned char* data() const noexcept { return m_compressedKey.data(); }
	size_t size() const noexcept { return m_compressedKey.size(); }

	void Serialize(Serializer& serializer) const
	{
		serializer.AppendBigInteger(m_compressedKey);
	}

	static PublicKey Deserialize(ByteBuffer& byteBuffer)
	{
		CBigInteger<33> compressedKey = byteBuffer.ReadBigInteger<33>();
		return PublicKey(std::move(compressedKey));
	}

private:
	CBigInteger<33> m_compressedKey;
};