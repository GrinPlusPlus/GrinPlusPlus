#pragma once

#include <Config/Config.h>
#include <Database/BlockDb.h>
#include <Core/BlockHeader.h>
#include <unordered_map>

class BlockStore
{
public:
	BlockStore(const Config& config, IBlockDB& blockDB);
	~BlockStore();

	void LoadHeaders(const std::vector<Hash>& hashes);
	std::unique_ptr<BlockHeader> GetBlockHeaderByHash(const Hash& hash) const;

	bool AddHeader(const BlockHeader& blockHeader);
	void AddHeaders(const std::vector<BlockHeader>& blockHeaders);

	bool AddBlock(const FullBlock& block);
	std::unique_ptr<FullBlock> GetBlockByHash(const Hash& hash) const;

	inline IBlockDB& GetBlockDB() { return m_blockDB; }

private:
	const Config& m_config;
	IBlockDB& m_blockDB;

	std::unordered_map<Hash, BlockHeader*> m_blockHeadersByHash;
};