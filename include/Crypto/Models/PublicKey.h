#pragma once

#include <Crypto/Models/BigInteger.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

class PublicKey : public Traits::IPrintable
{
public:
	PublicKey() = default;
	PublicKey(CBigInteger<33>&& compressedKey)
		: m_compressedKey(std::move(compressedKey)) { }

	static PublicKey FromHex(const std::string& hex) { return PublicKey(CBigInteger<33>::FromHex(hex)); }

	bool operator==(const PublicKey& rhs) const noexcept { return m_compressedKey == rhs.m_compressedKey; }

	const CBigInteger<33>& GetCompressedBytes() const noexcept { return m_compressedKey; }
	const std::vector<unsigned char>& GetCompressedVec() const noexcept { return m_compressedKey.GetData(); }
	std::string ToHex() const noexcept { return m_compressedKey.ToHex(); }

	const std::vector<uint8_t>& vec() const noexcept { return m_compressedKey.GetData(); }
	unsigned char* data() noexcept { return m_compressedKey.data(); }
	const unsigned char* data() const noexcept { return m_compressedKey.data(); }
	size_t size() const noexcept { return m_compressedKey.size(); }

	std::vector<uint8_t>::const_iterator cbegin() const noexcept { return GetCompressedVec().cbegin(); }
	std::vector<uint8_t>::const_iterator cend() const noexcept { return GetCompressedVec().cend(); }

	void Serialize(Serializer& serializer) const
	{
		serializer.AppendBigInteger(m_compressedKey);
	}

	static PublicKey Deserialize(ByteBuffer& byteBuffer)
	{
		CBigInteger<33> compressedKey = byteBuffer.ReadBigInteger<33>();
		return PublicKey(std::move(compressedKey));
	}

	std::string Format() const final { return "PublicKey{" + ToHex() + "}"; }

private:
	CBigInteger<33> m_compressedKey;
};