#pragma once

#include <Core/FullBlock.h>

class Environment
{
public:
	Environment(const FullBlock& block, const std::vector<uint8_t>& magicBytes, const uint32_t publicKeyVersion, const uint32_t privateKeyVersion)
		: m_block(std::move(block)), m_magicBytes(magicBytes), m_publicKeyVersion(publicKeyVersion), m_privateKeyVersion(privateKeyVersion)
	{

	}

	inline const FullBlock& GetGenesisBlock() const { return m_block; }
	inline const Hash& GetGenesisHash() const { return m_block.GetBlockHeader().GetHash(); }
	inline const std::vector<uint8_t>& GetMagicBytes() const { return m_magicBytes; }
	inline uint32_t GetPublicKeyVersion() const { return m_publicKeyVersion; }
	inline uint32_t GetPrivateKeyVersion() const { return m_privateKeyVersion; }

private:
	FullBlock m_block;
	std::vector<uint8_t> m_magicBytes;
	uint32_t m_publicKeyVersion;
	uint32_t m_privateKeyVersion;
};