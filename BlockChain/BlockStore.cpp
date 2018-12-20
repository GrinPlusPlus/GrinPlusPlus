#include "BlockStore.h"

BlockStore::BlockStore(const Config& config, IBlockDB& blockDB)
	: m_config(config), m_blockDB(blockDB)
{

}

BlockStore::~BlockStore()
{
	for (auto iter = m_blockHeadersByHash.begin(); iter != m_blockHeadersByHash.end(); iter++)
	{
		delete iter->second;
	}
}

void BlockStore::LoadHeaders(const std::vector<Hash>& hashes)
{
	std::vector<BlockHeader*> blockHeaders = m_blockDB.LoadBlockHeaders(hashes);
	for (BlockHeader* pBlockHeader : blockHeaders)
	{
		const Hash& hash = pBlockHeader->GetHash();
		m_blockHeadersByHash.insert({ hash, pBlockHeader });
	}
}

std::unique_ptr<BlockHeader> BlockStore::GetBlockHeaderByHash(const Hash& hash)
{
	auto iter = m_blockHeadersByHash.find(hash);
	if (iter != m_blockHeadersByHash.cend())
	{
		return std::make_unique<BlockHeader>(*iter->second);
	}

	return m_blockDB.GetBlockHeader(hash); // TODO: Cache this
}

bool BlockStore::AddHeader(const BlockHeader& blockHeader)
{
	const Hash& hash = blockHeader.GetHash();

	auto iter = m_blockHeadersByHash.find(hash);
	if (iter == m_blockHeadersByHash.cend())
	{
		m_blockHeadersByHash[hash] = new BlockHeader(blockHeader);
		m_blockDB.AddBlockHeader(blockHeader);

		return true;
	}

	return false;
}

void BlockStore::AddHeaders(const std::vector<BlockHeader>& blockHeaders)
{
	std::vector<BlockHeader*> blockHeadersToAdd;
	blockHeadersToAdd.reserve(blockHeaders.size());

	for (const BlockHeader& blockHeader : blockHeaders)
	{
		const Hash& hash = blockHeader.GetHash();

		auto iter = m_blockHeadersByHash.find(hash);
		if (iter == m_blockHeadersByHash.cend())
		{
			BlockHeader* pHeader = new BlockHeader(blockHeader);
			m_blockHeadersByHash[hash] = pHeader;
			blockHeadersToAdd.push_back(pHeader);
		}
	}

	m_blockDB.AddBlockHeaders(blockHeadersToAdd);
}