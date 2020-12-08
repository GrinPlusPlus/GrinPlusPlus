#include <BlockChain/Chain.h>
#include <Common/Logger.h>
#include <Core/Exceptions/BlockChainException.h>

Chain::Chain(
	const EChainType chainType,
	std::shared_ptr<BlockIndexAllocator> pBlockIndexAllocator,
	std::shared_ptr<DataFile<32>> pDataFile,
	std::vector<std::shared_ptr<const BlockIndex>>&& indices)
	: m_chainType(chainType),
	m_pBlockIndexAllocator(pBlockIndexAllocator),
	m_dataFile(pDataFile),
	m_dataFileWriter(),
	m_indices(std::move(indices)),
	m_height(m_indices.size() - 1)
{

}

std::shared_ptr<Chain> Chain::Load(
	std::shared_ptr<BlockIndexAllocator> pBlockIndexAllocator,
	const EChainType chainType,
	const fs::path& path,
	std::shared_ptr<const BlockIndex> pGenesisIndex)
{
	std::shared_ptr<DataFile<32>> pDataFile = DataFile<32>::Load(path);

	if (pDataFile->GetSize() == 0)
	{
		const auto bytes = pGenesisIndex->GetHash().GetData();
		pDataFile->AddData(bytes);
		pDataFile->Commit();
	}

	std::vector<std::shared_ptr<const BlockIndex>> indices;
	indices.push_back(pGenesisIndex);

	while (indices.size() < pDataFile->GetSize())
	{
		Hash hash = pDataFile->GetDataAt(indices.size());
		indices.emplace_back(pBlockIndexAllocator->GetOrCreateIndex(std::move(hash), indices.size()));
	}

	return std::shared_ptr<Chain>(new Chain(chainType, pBlockIndexAllocator, pDataFile, std::move(indices)));
}

std::shared_ptr<const BlockIndex> Chain::GetByHeight(const uint64_t height) const
{
	if (m_height >= height && m_indices.size() > height)
	{
		return m_indices[height];
	}

	return nullptr;
}

std::shared_ptr<const BlockIndex> Chain::AddBlock(const Hash& hash, const uint64_t height)
{
	if (height != m_height + 1)
	{
		LOG_ERROR_F("Tried to add block {} to height {}", hash, m_height + 1);
		throw BLOCK_CHAIN_EXCEPTION("Tried to add block to wrong height.");
	}

	SetDirty(true);

	std::shared_ptr<const BlockIndex> pBlockIndex = m_pBlockIndexAllocator->GetOrCreateIndex(hash, height);
	m_indices.push_back(pBlockIndex);

	const auto bytes = pBlockIndex->GetHash().GetData();
	m_dataFileWriter->AddData(bytes);

	++m_height;
	return pBlockIndex;
}

void Chain::Rewind(const uint64_t lastHeight)
{
	if (m_height < lastHeight)
	{
		throw BLOCK_CHAIN_EXCEPTION("Tried to rewind forward.");
	}

	if (m_height > lastHeight)
	{
		SetDirty(true);
		m_indices.erase(m_indices.begin() + lastHeight + 1, m_indices.end());

		m_dataFileWriter->Rewind(lastHeight + 1);
		m_height = lastHeight;
	}
}

void Chain::Commit()
{
	if (IsDirty())
	{
		m_dataFileWriter->Commit();
	}

	SetDirty(false);
}

void Chain::Rollback() noexcept
{
	if (IsDirty())
	{
		m_dataFileWriter->Rollback();
		m_indices.clear();

		while (m_indices.size() < m_dataFileWriter->GetSize())
		{
			Hash hash = m_dataFileWriter->GetDataAt(m_indices.size());
			m_indices.push_back(m_pBlockIndexAllocator->GetOrCreateIndex(std::move(hash), m_indices.size()));
		}

		m_height = m_indices.size() - 1;
	}

	SetDirty(false);
}

void Chain::OnInitWrite(const bool /*batch*/)
{
	SetDirty(false);
	m_dataFileWriter = m_dataFile.BatchWrite();
}

void Chain::OnEndWrite()
{
	m_dataFileWriter.Clear();
}