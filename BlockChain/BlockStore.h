#pragma once

#include <Config/Config.h>
#include <Database/BlockDb.h>
#include <Core/BlockHeader.h>
#include <map>

// TODO: Move to Database
class BlockStore
{
public:
	BlockStore(const Config& config, IBlockDB& blockDB);
	~BlockStore();

	void LoadHeaders(const std::vector<Hash>& hashes);
	std::unique_ptr<BlockHeader> GetBlockHeaderByHash(const Hash& hash);

	bool AddHeader(const BlockHeader& blockHeader);
	void AddHeaders(const std::vector<BlockHeader>& blockHeaders);

private:
	const Config& m_config;
	IBlockDB& m_blockDB;

	std::map<Hash, BlockHeader*> m_blockHeadersByHash;
};