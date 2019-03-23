#pragma once

#include <Crypto/BigInteger.h>
#include <Crypto/PublicKey.h>
#include <Crypto/SecretKey.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Serialization/ByteBuffer.h>

//
// Represents a BIP32 extended key.
//
class IExtendedKey // TODO: Just make this abstract and have a PrivateExtKey and PublicExtKey derivation
{
public:
	IExtendedKey(const uint32_t network, const uint8_t depth, const uint32_t parentFingerprint, const uint32_t childNumber, SecretKey&& chainCode, PublicKey&& keyBytes)
		: m_network(network),
		m_depth(depth),
		m_parentFingerprint(parentFingerprint),
		m_childNumber(childNumber),
		m_chainCode(std::move(chainCode)),
		m_keyBytes(std::move(keyBytes))
	{

	}

	//
	// 4 byte: version bytes (mainnet: TODO: public, TODO: private; floonet: 0x033C08DF public, 0x033C04A4 private)
	// 1 byte: depth: 0x00 for master nodes, 0x01 for level-1 derived keys, ....
	// 4 bytes: the fingerprint of the parent's key (0x00000000 if master key)
	// 4 bytes: child number. This is ser32(i) for i in xi = xpar/i, with xi the key being serialized. (0x00000000 if master key)
	// 32 bytes: the chain code
	// 33 bytes: the public key or private key data (serP(K) for public keys, 0x00 || ser256(k) for private keys)
	//
	void Serialize(Serializer& serializer) const
	{
		serializer.Append<uint32_t>(m_network);
		serializer.Append<uint8_t>(m_depth);
		serializer.Append<uint32_t>(m_parentFingerprint);
		serializer.Append<uint32_t>(m_childNumber);
		serializer.AppendBigInteger<32>(m_chainCode.GetBytes());
		m_keyBytes.Serialize(serializer);
	}

	inline uint32_t GetNetwork() const { return m_network; }
	inline uint8_t GetDepth() const { return m_depth; }
	inline uint32_t GetParentFingerprint() const { return m_parentFingerprint; }
	inline uint32_t GetChildNumber() const { return m_childNumber; }
	inline const SecretKey& GetChainCode() const { return m_chainCode; }

protected:
	inline const PublicKey& GetKeyBytes() const { return m_keyBytes; }

private:
	uint32_t m_network;
	uint8_t m_depth;
	uint32_t m_parentFingerprint;
	uint32_t m_childNumber;
	SecretKey m_chainCode;
	PublicKey m_keyBytes;
};