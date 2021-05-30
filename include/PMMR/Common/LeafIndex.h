#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <PMMR/Common/Index.h>
#include <Common/Util/BitUtil.h>

class LeafIndex : public Traits::IPrintable
{
public:
    LeafIndex() noexcept
        : m_leafIndex(0), m_nodeIndex(0, 0) { }
    LeafIndex(const uint64_t leafIndex, const uint64_t position)
        : m_leafIndex(leafIndex), m_nodeIndex(position, 0) { }
    virtual ~LeafIndex() = default;

    static LeafIndex At(const uint64_t leafIndex) noexcept
    {
        return LeafIndex(leafIndex, (2 * leafIndex) - BitUtil::CountBitsSet(leafIndex));
    }

    static LeafIndex AtPos(const uint64_t mmr_idx) noexcept
    {
        return LeafIndex::From(Index::At(mmr_idx));
    }

    static LeafIndex From(const Index& mmr_idx)
    {
        return LeafIndex(mmr_idx.GetLeafIndex(), mmr_idx.GetPosition());
    }

    operator uint64_t() const noexcept { return m_leafIndex; }

    bool operator<(const LeafIndex& rhs) const noexcept { return m_leafIndex < rhs.m_leafIndex; }
    bool operator>(const LeafIndex& rhs) const noexcept { return m_leafIndex > rhs.m_leafIndex; }
    bool operator==(const LeafIndex& rhs) const noexcept { return m_leafIndex == rhs.m_leafIndex; }
    bool operator!=(const LeafIndex& rhs) const noexcept { return m_leafIndex != rhs.m_leafIndex; }
    bool operator<=(const LeafIndex& rhs) const noexcept { return m_leafIndex <= rhs.m_leafIndex; }
    bool operator>=(const LeafIndex& rhs) const noexcept { return m_leafIndex >= rhs.m_leafIndex; }

    bool operator<(const uint64_t rhs) const noexcept { return m_leafIndex < rhs; }
    bool operator>(const uint64_t rhs) const noexcept { return m_leafIndex > rhs; }
    bool operator==(const uint64_t rhs) const noexcept { return m_leafIndex == rhs; }
    bool operator!=(const uint64_t rhs) const noexcept { return m_leafIndex != rhs; }
    bool operator<=(const uint64_t rhs) const noexcept { return m_leafIndex <= rhs; }
    bool operator>=(const uint64_t rhs) const noexcept { return m_leafIndex >= rhs; }

    LeafIndex& operator++()
    {
        *this = this->Next();
        return *this;
    }

    LeafIndex operator++(int)
    {
        LeafIndex temp(*this);
        *this = this->Next();
        return temp;
    }

    LeafIndex& operator--()
    {
        *this = this->Prev();
        return *this;
    }

    uint64_t Get() const noexcept { return m_leafIndex; }
    const Index& GetIndex() const noexcept { return m_nodeIndex; }
    uint64_t GetPosition() const noexcept { return m_nodeIndex.GetPosition(); }
    LeafIndex Next() const noexcept { return LeafIndex::At(m_leafIndex + 1); }
    LeafIndex Prev() const noexcept { return LeafIndex::At(m_leafIndex - 1); }

    std::string Format() const noexcept final { return "LeafIndex(" + std::to_string(m_leafIndex) + ")"; }

private:
    uint64_t m_leafIndex;
    Index m_nodeIndex;
};