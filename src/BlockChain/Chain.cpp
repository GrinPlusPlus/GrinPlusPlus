#include "Chain.h"
#include "ChainStore.h"

Chain::Chain(const EChainType chainType, std::shared_ptr<DataFile<32>> pDataFile, std::vector<BlockIndex*>&& indices)
	: m_chainType(chainType), m_pDataFile(pDataFile), m_transaction(), m_indices(std::move(indices)), m_height(m_indices.size() - 1)
{

}

std::shared_ptr<Chain> Chain::Load(BlockIndexAllocator& blockIndexAllocator, const EChainType chainType, const std::string& path, BlockIndex* pGenesisIndex)
{
	std::shared_ptr<DataFile<32>> pDataFile = DataFile<32>::Load(path);

	if (pDataFile->GetSize() == 0)
	{
		const auto& bytes = pGenesisIndex->GetHash().GetData();
		pDataFile->AddData(bytes);
		pDataFile->Commit();
	}

	std::vector<BlockIndex*> indices;
	pGenesisIndex->AddChainType(chainType);
	indices.push_back(pGenesisIndex);

	size_t nextHeight = 1;
	BlockIndex* pPrevious = pGenesisIndex;
	while (nextHeight < pDataFile->GetSize())
	{
		const Hash hash(pDataFile->GetDataAt(nextHeight));

		BlockIndex* pBlockIndex = blockIndexAllocator.GetOrCreateIndex(hash, nextHeight, pPrevious);

		pBlockIndex->AddChainType(chainType);
		indices.push_back(pBlockIndex);

		pPrevious = pBlockIndex;
		++nextHeight;
	}

	return std::make_shared<Chain>(Chain(chainType, pDataFile, std::move(indices)));
}

BlockIndex* Chain::GetByHeight(const uint64_t height)
{
	if (m_height >= height)
	{
		return m_indices[height];
	}

	return nullptr;
}

const BlockIndex* Chain::GetByHeight(const uint64_t height) const
{
	if (m_height >= height)
	{
		return m_indices[height];
	}

	return nullptr;
}

BlockIndex* Chain::GetTip()
{
	return m_indices[m_height];
}

const BlockIndex* Chain::GetTip() const
{
	return m_indices[m_height];
}

bool Chain::AddBlock(BlockIndex* pBlockIndex)
{
	if (pBlockIndex->GetHeight() == (m_height + 1) && pBlockIndex->GetPrevious() == GetTip())
	{
		pBlockIndex->AddChainType(m_chainType);
		m_indices.push_back(pBlockIndex);

		const auto& bytes = pBlockIndex->GetHash().GetData();
		GetTransaction()->AddData(bytes);
		m_height++;
		return true;
	}

	return false;
}

bool Chain::Rewind(const uint64_t lastHeight)
{
	if (m_height < lastHeight)
	{
		return false;
	}

	if (m_height > lastHeight)
	{
		for (uint64_t i = lastHeight + 1; i <= m_height; i++)
		{
			BlockIndex* pBlockIndex = m_indices[i];
			pBlockIndex->RemoveChainType(m_chainType);
			if (pBlockIndex->IsSafeToDelete())
			{
				delete pBlockIndex;
			}
		}

		m_indices.erase(m_indices.begin() + lastHeight + 1, m_indices.end());

		GetTransaction()->Rewind(lastHeight + 1);
		m_height = lastHeight;
	}

	return true;
}

bool Chain::Flush()
{
	GetTransaction()->Commit();
	return true;
}

Writer<DataFile<32>> Chain::GetTransaction()
{
	if (m_transaction.IsNull())
	{
		m_transaction = m_pDataFile.BatchWrite();
	}

	return m_transaction;
}