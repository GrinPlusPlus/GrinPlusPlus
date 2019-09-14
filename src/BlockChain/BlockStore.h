#pragma once

#include <Config/Config.h>
#include <Database/BlockDb.h>
#include <Core/Models/BlockHeader.h>
#include <unordered_map>

// TODO: This was meant to be a wrapper with caching, but no longer contains caching. Remove this class
class BlockStore
{
public:
	BlockStore(const Config& config, IBlockDB& blockDB);
	~BlockStore();

	std::unique_ptr<BlockHeader> GetBlockHeaderByHash(const Hash& hash) const;

	bool AddHeader(const BlockHeader& blockHeader);
	void AddHeaders(const std::vector<BlockHeader>& blockHeaders);

	bool AddBlock(const FullBlock& block);
	std::unique_ptr<FullBlock> GetBlockByHash(const Hash& hash) const;

	inline IBlockDB& GetBlockDB() { return m_blockDB; }

private:
	const Config& m_config;
	IBlockDB& m_blockDB;
};