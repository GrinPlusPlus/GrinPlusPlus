#pragma once

#include "HashFile.h"
#include "LeafSet.h"
#include "MMR.h"
#include "PruneList.h"
#include <CRoaring/roaring.hh>
#include <Core/DataFile.h>

#include "MMRHashUtil.h"
#include "MMRUtil.h"

#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Infrastructure/Logger.h>

template <size_t DATA_SIZE, class DATA_TYPE> class PruneableMMR : public MMR
{
  public:
    PruneableMMR(HashFile *pHashFile, LeafSet &&leafSet, PruneList &&pruneList, DataFile<DATA_SIZE> *pDataFile)
        : m_pHashFile(pHashFile), m_leafSet(std::move(leafSet)), m_pruneList(std::move(pruneList)),
          m_pDataFile(pDataFile)
    {
    }

    virtual ~PruneableMMR()
    {
        delete m_pHashFile;
        m_pHashFile = nullptr;

        delete m_pDataFile;
        m_pDataFile = nullptr;
    }

    bool Append(const DATA_TYPE &object)
    {
        // Add to LeafSet
        const uint64_t totalShift = m_pruneList.GetTotalShift();
        const uint64_t mmrIndex = m_pHashFile->GetSize() + totalShift;
        m_leafSet.Add((uint32_t)mmrIndex);

        // Add to data file
        Serializer serializer;
        object.Serialize(serializer);
        m_pDataFile->AddData(serializer.GetBytes());

        // Add hashes
        MMRHashUtil::AddHashes(*m_pHashFile, serializer.GetBytes(), &m_pruneList);

        return true;
    }

    bool Remove(const uint64_t mmrIndex)
    {
        LOG_TRACE_F("Spending output at index (%llu)", mmrIndex);

        if (!MMRUtil::IsLeaf(mmrIndex))
        {
            LOG_WARNING_F("Output is not a leaf (%llu)", mmrIndex);
            return false;
        }

        if (!m_leafSet.Contains(mmrIndex))
        {
            LOG_WARNING_F("LeafSet does not contain output (%llu)", mmrIndex);
            return false;
        }

        m_leafSet.Remove((uint32_t)mmrIndex);

        return true;
    }

    bool Rewind(const uint64_t size, const Roaring &leavesToAdd)
    {
        const bool hashRewind = m_pHashFile->Rewind(size - m_pruneList.GetShift(size - 1));
        const bool dataRewind =
            m_pDataFile->Rewind(MMRUtil::GetNumLeaves(size - 1) - m_pruneList.GetLeafShift(size - 1));
        m_leafSet.Rewind(size, leavesToAdd);

        return hashRewind && dataRewind;
    }

    virtual Hash Root(const uint64_t size) const override final
    {
        return MMRHashUtil::Root(*m_pHashFile, size, &m_pruneList);
    }

    virtual uint64_t GetSize() const override final
    {
        const uint64_t totalShift = m_pruneList.GetTotalShift();

        return totalShift + m_pHashFile->GetSize();
    }

    virtual std::unique_ptr<Hash> GetHashAt(const uint64_t mmrIndex) const override final
    {
        Hash hash = MMRHashUtil::GetHashAt(*m_pHashFile, mmrIndex, &m_pruneList);
        if (hash == ZERO_HASH)
        {
            return std::unique_ptr<Hash>(nullptr);
        }

        return std::make_unique<Hash>(std::move(hash));
    }

    virtual std::vector<Hash> GetLastLeafHashes(const uint64_t numHashes) const override final
    {
        return MMRHashUtil::GetLastLeafHashes(*m_pHashFile, &m_leafSet, &m_pruneList, numHashes);
    }

    bool IsUnpruned(const uint64_t mmrIndex) const
    {
        if (MMRUtil::IsLeaf(mmrIndex))
        {
            if (mmrIndex < GetSize())
            {
                return m_leafSet.Contains(mmrIndex);
            }
        }

        return false;
    }

    std::unique_ptr<DATA_TYPE> GetAt(const uint64_t mmrIndex) const
    {
        if (IsUnpruned(mmrIndex))
        {
            const uint64_t shift = m_pruneList.GetLeafShift(mmrIndex);
            const uint64_t numLeaves = MMRUtil::GetNumLeaves(mmrIndex);
            const uint64_t shiftedIndex = ((numLeaves - 1) - shift);

            std::vector<unsigned char> data;
            m_pDataFile->GetDataAt(shiftedIndex, data);

            if (data.size() == DATA_SIZE)
            {
                ByteBuffer byteBuffer(data);
                return std::make_unique<DATA_TYPE>(DATA_TYPE::Deserialize(byteBuffer));
            }
        }

        return std::unique_ptr<DATA_TYPE>(nullptr);
    }

    virtual bool Flush() override final
    {
        LOG_TRACE_F("Flushing with size (%llu)", GetSize());
        const bool hashFlush = m_pHashFile->Flush();
        const bool dataFlush = m_pDataFile->Flush();
        const bool leafSetFlush = m_leafSet.Flush();

        return hashFlush && dataFlush && leafSetFlush;
    }

    virtual bool Discard() override final
    {
        LOG_INFO("Discarding changes since last flush");
        const bool hashDiscard = m_pHashFile->Discard();
        const bool dataDiscard = m_pDataFile->Discard();
        m_leafSet.Discard();

        // TODO: const bool pruneDiscard = m_pruneList.Discard();

        return hashDiscard && dataDiscard;
    }

  private:
    HashFile *m_pHashFile;
    LeafSet m_leafSet;
    PruneList m_pruneList;
    DataFile<DATA_SIZE> *m_pDataFile;
};