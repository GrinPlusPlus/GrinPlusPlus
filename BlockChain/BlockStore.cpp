#include "BlockStore.h"

BlockStore::BlockStore(const Config& config, IBlockDB& blockDB)
	: m_config(config), m_blockDB(blockDB)
{

}

BlockStore::~BlockStore()
{

}

std::unique_ptr<BlockHeader> BlockStore::GetBlockHeaderByHash(const Hash& hash) const
{
	return m_blockDB.GetBlockHeader(hash);
}

bool BlockStore::AddHeader(const BlockHeader& blockHeader)
{
	m_blockDB.AddBlockHeader(blockHeader);
	return true;
}

void BlockStore::AddHeaders(const std::vector<BlockHeader>& blockHeaders)
{
	m_blockDB.AddBlockHeaders(blockHeaders);
}

bool BlockStore::AddBlock(const FullBlock& block)
{
	m_blockDB.AddBlock(block);
	return true;
}

std::unique_ptr<FullBlock> BlockStore::GetBlockByHash(const Hash& hash) const
{
	return m_blockDB.GetBlock(hash);
}