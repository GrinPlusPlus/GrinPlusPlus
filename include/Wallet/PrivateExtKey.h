#pragma once

#include "ExtendedKey.h"

class PrivateExtKey : public IExtendedKey
{
public:
	static PrivateExtKey Create(const uint32_t network, const uint8_t depth, const uint32_t parentFingerprint, const uint32_t childNumber, SecretKey&& chainCode, SecretKey&& privateKey)
	{
		std::vector<unsigned char> keyBytes;
		keyBytes.reserve(33);
		keyBytes.push_back(0);
		keyBytes.insert(keyBytes.end(), privateKey.GetBytes().GetData().cbegin(), privateKey.GetBytes().GetData().cend());
		return PrivateExtKey(network, depth, parentFingerprint, childNumber, std::move(chainCode), CBigInteger<33>(std::move(keyBytes)), std::move(privateKey));
	}

	inline const SecretKey& GetPrivateKey() const { return m_privateKey; }
	
	static PrivateExtKey Deserialize(ByteBuffer& byteBuffer)
	{ 
		const uint32_t network = byteBuffer.ReadU32();
		const uint8_t depth = byteBuffer.ReadU8();
		const uint32_t parentFingerprint = byteBuffer.ReadU32();
		const uint32_t childNumber = byteBuffer.ReadU32();
		SecretKey chainCode = byteBuffer.ReadBigInteger<32>();
		CBigInteger<33> keyBytes = byteBuffer.ReadBigInteger<33>();

		std::vector<unsigned char> privateKeyBytes(keyBytes.GetData().cbegin() + 1, keyBytes.GetData().cend());
		SecretKey privateKey(std::move(privateKeyBytes));

		return PrivateExtKey(network, depth, parentFingerprint, childNumber, std::move(chainCode), std::move(keyBytes), std::move(privateKey));
	}

private:
	PrivateExtKey(const uint32_t network, const uint8_t depth, const uint32_t parentFingerprint, const uint32_t childNumber, SecretKey&& chainCode, CBigInteger<33>&& keyBytes, SecretKey&& privateKey)
		: IExtendedKey(network, depth, parentFingerprint, childNumber, std::move(chainCode), std::move(keyBytes)), 
		m_privateKey(std::move(privateKey))
	{

	}

	SecretKey m_privateKey;
};