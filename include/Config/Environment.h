#pragma once

#include <Config/EnvironmentType.h>
#include <Core/Models/FullBlock.h>

class Environment
{
public:
	Environment(const EEnvironmentType environmentType, const FullBlock& block, const std::vector<uint8_t>& magicBytes, const uint16_t port)
		: m_environmentType(environmentType), m_block(std::move(block)), m_magicBytes(magicBytes), m_port(port)
	{

	}

	inline const FullBlock& GetGenesisBlock() const { return m_block; }
	inline const Hash& GetGenesisHash() const { return m_block.GetBlockHeader().GetHash(); }
	inline const std::vector<uint8_t>& GetMagicBytes() const { return m_magicBytes; }
	inline uint16_t GetP2PPort() const { return m_port; }
	inline EEnvironmentType GetEnvironmentType() const { return m_environmentType; }
	inline bool IsMainnet() const { return m_environmentType == EEnvironmentType::MAINNET; }

private:
	EEnvironmentType m_environmentType;
	FullBlock m_block;
	std::vector<uint8_t> m_magicBytes;
	uint16_t m_port;
	uint32_t m_publicKeyVersion;
	uint32_t m_privateKeyVersion;
};