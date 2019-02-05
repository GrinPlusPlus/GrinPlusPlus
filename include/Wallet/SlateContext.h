#pragma once

#include <Crypto.h>
#include <Serialization/Serializer.h>
#include <Serialization/ByteBuffer.h>

class SlateContext
{
public:
	SlateContext(BlindingFactor&& secretKey, BlindingFactor&& secretNonce)
		: m_secretKey(std::move(secretKey)), m_secretNonce(std::move(secretNonce))
	{

	}

	inline const BlindingFactor& GetSecretKey() const { return m_secretKey; }
	inline const BlindingFactor& GetSecretNonce() const { return m_secretNonce; }

	void Serialize(Serializer& serializer) const
	{
		m_secretKey.Serialize(serializer);
		m_secretNonce.Serialize(serializer);
	}

	static SlateContext Deserialize(ByteBuffer& byteBuffer)
	{
		BlindingFactor secretKey = BlindingFactor::Deserialize(byteBuffer);
		BlindingFactor secretNonce = BlindingFactor::Deserialize(byteBuffer);

		return SlateContext(std::move(secretKey), std::move(secretNonce));
	}

private:
	BlindingFactor m_secretKey;
	BlindingFactor m_secretNonce;
};