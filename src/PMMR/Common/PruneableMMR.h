#pragma once

#include "MMR.h"
#include "HashFile.h"
#include "LeafSet.h"
#include "PruneList.h"

#include "MMRUtil.h"
#include "MMRHashUtil.h"

#include <Core/File/DataFile.h>
#include <Roaring.h>
#include <Core/Exceptions/TxHashSetException.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Traits/Lockable.h>
#include <Infrastructure/Logger.h>

template<size_t DATA_SIZE, class DATA_TYPE>
class PruneableMMR : public MMR, public Traits::IBatchable
{
public:
	PruneableMMR(
		std::shared_ptr<HashFile> pHashFile,
		std::shared_ptr<LeafSet> pLeafSet,
		std::shared_ptr<PruneList> pPruneList,
		std::shared_ptr<DataFile<DATA_SIZE>> pDataFile)
		: m_pHashFile(pHashFile),
		m_pLeafSet(pLeafSet),
		m_pPruneList(pPruneList),
		m_pDataFile(pDataFile)
	{

	}

	virtual ~PruneableMMR() = default;

	void Append(const DATA_TYPE& object)
	{
		SetDirty(true);

		// Add to LeafSet
		const uint64_t totalShift = m_pPruneList->GetTotalShift();
		const uint64_t mmrIndex = m_pHashFile->GetSize() + totalShift;
		m_pLeafSet->Add(MMRUtil::GetLeafIndex(mmrIndex));

		// Add to data file
		Serializer serializer;
		object.Serialize(serializer);
		m_pDataFile->AddData(serializer.GetBytes());

		// Add hashes
		MMRHashUtil::AddHashes(m_pHashFile, serializer.GetBytes(), m_pPruneList);
	}

	void Remove(const uint64_t mmrIndex)
	{
		LOG_TRACE_F("Spending output at index ({})", mmrIndex);
		SetDirty(true);

		if (!MMRUtil::IsLeaf(mmrIndex))
		{
			LOG_WARNING_F("Output is not a leaf ({})", mmrIndex);
			throw TXHASHSET_EXCEPTION(StringUtil::Format("Output is not a leaf ({})", mmrIndex));
		}

		const uint64_t leafIndex = MMRUtil::GetLeafIndex(mmrIndex);
		if (!m_pLeafSet->Contains(leafIndex))
		{
			LOG_WARNING_F("LeafSet does not contain output ({})", mmrIndex);
			throw TXHASHSET_EXCEPTION(StringUtil::Format("LeafSet does not contain output ({})", mmrIndex));
		}

		m_pLeafSet->Remove(leafIndex);
	}

	void Rewind(const uint64_t size, const std::vector<uint64_t>& leavesToAdd)
	{
		SetDirty(true);

		m_pHashFile->Rewind(size - m_pPruneList->GetShift(size - 1));
		m_pDataFile->Rewind(MMRUtil::GetNumLeaves(size - 1) - m_pPruneList->GetLeafShift(size - 1));
		m_pLeafSet->Rewind(size, leavesToAdd);
	}

	Hash Root(const uint64_t size) const final
	{
		return MMRHashUtil::Root(m_pHashFile, size, m_pPruneList);
	}

	Hash UBMTRoot(const uint64_t size) const
	{
		return m_pLeafSet->Root(size);
	}

	uint64_t GetSize() const final
	{
		const uint64_t totalShift = m_pPruneList->GetTotalShift();

		return totalShift + m_pHashFile->GetSize();
	}

	std::unique_ptr<Hash> GetHashAt(const uint64_t mmrIndex) const final
	{
		Hash hash = MMRHashUtil::GetHashAt(m_pHashFile, mmrIndex, m_pPruneList);
		if (hash == ZERO_HASH)
		{
			return std::unique_ptr<Hash>(nullptr);
		}

		return std::make_unique<Hash>(std::move(hash));
	}

	std::vector<Hash> GetLastLeafHashes(const uint64_t numHashes) const final
	{
		return MMRHashUtil::GetLastLeafHashes(m_pHashFile, m_pLeafSet, m_pPruneList, numHashes);
	}

	bool IsUnpruned(const uint64_t mmrIndex) const
	{
		if (MMRUtil::IsLeaf(mmrIndex))
		{
			if (mmrIndex < GetSize())
			{
				return m_pLeafSet->Contains(MMRUtil::GetLeafIndex(mmrIndex));
			}
		}

		return false;
	}

	std::unique_ptr<DATA_TYPE> GetAt(const uint64_t mmrIndex) const
	{
		if (IsUnpruned(mmrIndex))
		{
			const uint64_t shift = m_pPruneList->GetLeafShift(mmrIndex);
			const uint64_t numLeaves = MMRUtil::GetNumLeaves(mmrIndex);
			const uint64_t shiftedIndex = ((numLeaves - 1) - shift);

			try
			{
				std::vector<unsigned char> data = m_pDataFile->GetDataAt(shiftedIndex);
				if (data.size() == DATA_SIZE)
				{
					ByteBuffer byteBuffer(std::move(data));
					return std::make_unique<DATA_TYPE>(DATA_TYPE::Deserialize(byteBuffer));
				}
			}
			catch (FileException&)
			{
				return std::unique_ptr<DATA_TYPE>(nullptr);
			}
		}

		return std::unique_ptr<DATA_TYPE>(nullptr);
	}

	void Commit() final
	{
		if (IsDirty())
		{
			LOG_TRACE_F("Flushing with size ({})", GetSize());
			m_pHashFile->Commit();
			m_pDataFile->Commit();
			m_pLeafSet->Commit();
			SetDirty(false);
		}
	}

	void Rollback() noexcept final
	{
		if (IsDirty())
		{
			LOG_INFO("Discarding changes since last flush");
			m_pHashFile->Rollback();
			m_pDataFile->Rollback();
			m_pLeafSet->Rollback();
			SetDirty(false);
		}
	}

private:
	std::shared_ptr<HashFile> m_pHashFile;
	std::shared_ptr<LeafSet> m_pLeafSet;
	std::shared_ptr<PruneList> m_pPruneList;
	std::shared_ptr<DataFile<DATA_SIZE>> m_pDataFile;
};