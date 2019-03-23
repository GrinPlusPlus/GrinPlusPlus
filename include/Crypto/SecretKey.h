#pragma once

#include <Crypto/BigInteger.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

class SecretKey
{
public:
	SecretKey(CBigInteger<32>&& seed)
		: m_seed(std::move(seed))
	{

	}

	~SecretKey()
	{
		m_seed.erase();
	}

	inline const CBigInteger<32>& GetBytes() const { return m_seed; }

	inline const unsigned char* data() const { return m_seed.GetData().data(); }
	inline size_t size() const { return m_seed.GetData().size(); }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const
	{
		serializer.AppendBigInteger<32>(m_seed);
	}

	static SecretKey Deserialize(ByteBuffer& byteBuffer)
	{
		return SecretKey(byteBuffer.ReadBigInteger<32>());
	}

private:
	CBigInteger<32> m_seed;
};