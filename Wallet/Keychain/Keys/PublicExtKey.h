#pragma once

#include "ExtendedKey.h"

class PublicExtKey : public IExtendedKey
{
public:
	PublicExtKey(const uint32_t network, const uint8_t depth, const uint32_t parentFingerprint, const uint32_t childNumber, CBigInteger<32>&& chainCode, CBigInteger<33>&& keyBytes)
		: IExtendedKey(network, depth, parentFingerprint, childNumber, std::move(chainCode), std::move(keyBytes))
	{

	}

	inline const CBigInteger<33>& GetPublicKey() const { return GetKeyBytes(); }
	
	static PublicExtKey Deserialize(ByteBuffer& byteBuffer)
	{ 
		const uint32_t network = byteBuffer.ReadU32();
		const uint8_t depth = byteBuffer.ReadU8();
		const uint32_t parentFingerprint = byteBuffer.ReadU32();
		const uint32_t childNumber = byteBuffer.ReadU32();
		CBigInteger<32> chainCode = byteBuffer.ReadBigInteger<32>();
		CBigInteger<33> compressedPublicKey = byteBuffer.ReadBigInteger<33>();
		return PublicExtKey(network, depth, parentFingerprint, childNumber, std::move(chainCode), std::move(compressedPublicKey));
	}
};