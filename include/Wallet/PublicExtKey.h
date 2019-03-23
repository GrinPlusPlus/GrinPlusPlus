#pragma once

#include "ExtendedKey.h"

class PublicExtKey : public IExtendedKey
{
public:
	PublicExtKey(const uint32_t network, const uint8_t depth, const uint32_t parentFingerprint, const uint32_t childNumber, SecretKey&& chainCode, PublicKey&& keyBytes)
		: IExtendedKey(network, depth, parentFingerprint, childNumber, std::move(chainCode), std::move(keyBytes))
	{

	}

	inline const PublicKey& GetPublicKey() const { return GetKeyBytes(); }
	
	static PublicExtKey Deserialize(ByteBuffer& byteBuffer)
	{ 
		const uint32_t network = byteBuffer.ReadU32();
		const uint8_t depth = byteBuffer.ReadU8();
		const uint32_t parentFingerprint = byteBuffer.ReadU32();
		const uint32_t childNumber = byteBuffer.ReadU32();
		SecretKey chainCode = byteBuffer.ReadBigInteger<32>();
		PublicKey compressedPublicKey = PublicKey::Deserialize(byteBuffer);
		return PublicExtKey(network, depth, parentFingerprint, childNumber, std::move(chainCode), std::move(compressedPublicKey));
	}
};