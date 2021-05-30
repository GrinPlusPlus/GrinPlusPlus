#pragma once

#include <Core/Traits/Serializable.h>
#include <Core/Traits/Hashable.h>
#include <Crypto/Hasher.h>
#include <PMMR/Common/MMRUtil.h>
#include <PMMR/Common/MMRHashUtil.h>
#include <type_traits>

class RootCalculator
{
public:
    template<class T, typename = std::enable_if_t<std::is_base_of_v<Traits::ISerializable, T>>>
    static Hash CalculateRoot(const std::vector<T>& leaves)
    {
        assert(!leaves.empty());

        std::vector<Hash> hashes;

        for (LeafIndex leaf_idx = LeafIndex::At(0); leaf_idx < leaves.size(); leaf_idx++) {
            Hash leaf_hash = Hasher::Blake2b(leaves[leaf_idx].SerializeWithIndex(leaf_idx.GetPosition()));
            hashes.push_back(std::move(leaf_hash));

            // Add parent hashes
            for (Index mmr_idx = leaf_idx.GetIndex() + 1; !mmr_idx.IsLeaf(); mmr_idx++) {
                Hash mmr_hash = MMRHashUtil::HashParentWithIndex(
                    hashes[mmr_idx.GetLeftChild().Get()],
                    hashes[mmr_idx.GetRightChild().Get()],
                    mmr_idx.Get()
                );
                hashes.push_back(std::move(mmr_hash));
            }
        }

        Hash hash = ZERO_HASH;
        const std::vector<uint64_t> peakIndices = MMRUtil::GetPeakIndices(hashes.size());
        for (auto iter = peakIndices.crbegin(); iter != peakIndices.crend(); iter++) {
            const Hash& peakHash = hashes[*iter];
            if (peakHash != ZERO_HASH) {
                if (hash == ZERO_HASH) {
                    hash = peakHash;
                } else {
                    hash = MMRHashUtil::HashParentWithIndex(peakHash, hash, hashes.size());
                }
            }
        }

        return hash;
    }
};