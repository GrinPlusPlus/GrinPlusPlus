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
#include <Common/Logger.h>

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
		Index mmr_idx = Index::At(m_pHashFile->GetSize() + m_pPruneList->GetTotalShift());
		m_pLeafSet->Add(LeafIndex::From(mmr_idx));

		// Add to data file
		std::vector<uint8_t> serialized = object.Serialized();
		m_pDataFile->AddData(serialized);

		// Add hashes
		MMRHashUtil::AddHashes(m_pHashFile, serialized, m_pPruneList);
	}

	void Remove(const LeafIndex& leaf_idx)
	{
		LOG_TRACE_F("Spending output at {}", leaf_idx);

		if (!m_pLeafSet->Contains(leaf_idx)) {
			LOG_WARNING_F("LeafSet does not contain output: {}", leaf_idx);
			throw TXHASHSET_EXCEPTION(StringUtil::Format("LeafSet does not contain output: {}", leaf_idx));
		}

		SetDirty(true);
		m_pLeafSet->Remove(leaf_idx);
	}

	void Rewind(const uint64_t num_leaves, const std::vector<uint64_t>& leavesToAdd)
	{
		SetDirty(true);

		LeafIndex next_leaf = LeafIndex::At(num_leaves);
		m_pHashFile->Rewind(next_leaf.GetPosition() - m_pPruneList->GetShift(next_leaf.GetIndex() - 1));
		m_pDataFile->Rewind(num_leaves - m_pPruneList->GetLeafShift(next_leaf.GetIndex() - 1));
		m_pLeafSet->Rewind(num_leaves, leavesToAdd);
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
		return m_pPruneList->GetTotalShift() + m_pHashFile->GetSize();
	}

	std::unique_ptr<Hash> GetHashAt(const Index& mmrIndex) const final
	{
		Hash hash = MMRHashUtil::GetHashAt(m_pHashFile, mmrIndex, m_pPruneList);
		if (hash == ZERO_HASH) {
			return std::unique_ptr<Hash>(nullptr);
		}

		return std::make_unique<Hash>(std::move(hash));
	}

	std::vector<Hash> GetLastLeafHashes(const uint64_t numHashes) const final
	{
		return MMRHashUtil::GetLastLeafHashes(m_pHashFile, m_pLeafSet, m_pPruneList, numHashes);
	}

	bool IsUnpruned(const LeafIndex& leaf_idx) const
	{
		return leaf_idx.GetPosition() < GetSize() && m_pLeafSet->Contains(leaf_idx);
	}

	std::unique_ptr<DATA_TYPE> GetAt(const LeafIndex& leaf_idx) const
	{
        if (IsUnpruned(leaf_idx)) {
            uint64_t shift = m_pPruneList->GetLeafShift(leaf_idx.GetIndex());
            uint64_t shifted_idx = leaf_idx.Get() - shift;

            try {
                std::vector<unsigned char> data = m_pDataFile->GetDataAt(shifted_idx);
                if (data.size() == DATA_SIZE) {
                    ByteBuffer byteBuffer(std::move(data));
                    return std::make_unique<DATA_TYPE>(DATA_TYPE::Deserialize(byteBuffer));
                }
            }
            catch (FileException&) {
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