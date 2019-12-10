#pragma once

#include <Config/EnvironmentType.h>
#include <Config/Genesis.h>

class Environment
{
public:
	//
	// Getters
	//
	const FullBlock& GetGenesisBlock() const { return m_block; }
	const Hash& GetGenesisHash() const { return m_block.GetBlockHeader()->GetHash(); }
	const std::vector<uint8_t>& GetMagicBytes() const { return m_magicBytes; }
	uint16_t GetP2PPort() const { return m_port; }
	EEnvironmentType GetEnvironmentType() const { return m_environmentType; }
	bool IsMainnet() const { return m_environmentType == EEnvironmentType::MAINNET; }

	//
	// Constructor
	//
	Environment(const EEnvironmentType environmentType)
		: m_environmentType(environmentType)
	{
		if (m_environmentType == EEnvironmentType::MAINNET)
		{
			m_port = 3414;
			m_magicBytes = { 97, 61 };
			m_block = Genesis::MAINNET_GENESIS;
		}
		else
		{
			m_port = 13414;
			m_magicBytes = { 83, 59 };
			m_block = Genesis::FLOONET_GENESIS;
		}
	}

private:
	EEnvironmentType m_environmentType;
	FullBlock m_block;
	std::vector<uint8_t> m_magicBytes;
	uint16_t m_port;
};