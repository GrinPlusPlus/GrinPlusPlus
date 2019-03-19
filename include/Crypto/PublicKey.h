#pragma once

#include <Crypto/BigInteger.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

class PublicKey
{
public:
	PublicKey(CBigInteger<33>&& compressedKey)
		: m_compressedKey(std::move(compressedKey))
	{

	}

	inline const CBigInteger<33>& GetCompressedBytes() const { return m_compressedKey; }

	inline const unsigned char* data() const { return m_compressedKey.data(); }
	inline size_t size() const { return m_compressedKey.size(); }

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