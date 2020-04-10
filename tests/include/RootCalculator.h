#pragma once

#include <Core/Traits/Serializable.h>
#include <Core/Traits/Hashable.h>
#include <Crypto/Hash.h>
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

        for (size_t leafIdx = 0; leafIdx < leaves.size(); leafIdx++)
        {
			uint64_t position = MMRUtil::GetPMMRIndex(leafIdx);
            hashes.push_back(Crypto::Blake2b(leaves[leafIdx].SerializeWithIndex(position)));

			const uint64_t nextLeafPosition = MMRUtil::GetPMMRIndex(leafIdx + 1);

			// Add parent hashes
			uint64_t peak = 1;
			while (++position < nextLeafPosition)
			{
				const uint64_t leftSiblingPosition = position - (2 * peak);

				const Hash& leftHash = hashes[leftSiblingPosition];
				const Hash& rightHash = hashes[position - 1];

				peak *= 2;

				hashes.push_back(MMRHashUtil::HashParentWithIndex(leftHash, rightHash, position));
			}
		}

		Hash hash = ZERO_HASH;
		const std::vector<uint64_t> peakIndices = MMRUtil::GetPeakIndices(hashes.size());
		for (auto iter = peakIndices.crbegin(); iter != peakIndices.crend(); iter++)
		{
			const Hash& peakHash = hashes[*iter];
			if (peakHash != ZERO_HASH)
			{
				if (hash == ZERO_HASH)
				{
					hash = peakHash;
				}
				else
				{
					hash = MMRHashUtil::HashParentWithIndex(peakHash, hash, hashes.size());
				}
			}
		}

		return hash;
    }
};