#pragma once

#include <Crypto/BigInteger.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

template<size_t NUM_BYTES>
class secret_key_t
{
public:
	secret_key_t(CBigInteger<NUM_BYTES>&& seed)
		: m_seed(std::move(seed))
	{

	}

	~secret_key_t()
	{
		m_seed.erase();
	}

	inline const CBigInteger<NUM_BYTES>& GetBytes() const { return m_seed; }
	inline const std::vector<unsigned char>& GetVec() const { return m_seed.GetData(); }

	inline const unsigned char* data() const { return m_seed.data(); }
	inline size_t size() const { return m_seed.size(); }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const
	{
		serializer.AppendBigInteger<NUM_BYTES>(m_seed);
	}

	static secret_key_t<NUM_BYTES> Deserialize(ByteBuffer& byteBuffer)
	{
		return secret_key_t<NUM_BYTES>(byteBuffer.ReadBigInteger<NUM_BYTES>());
	}

private:
	CBigInteger<NUM_BYTES> m_seed;
};

typedef secret_key_t<32> SecretKey;
typedef secret_key_t<64> SecretKey64;