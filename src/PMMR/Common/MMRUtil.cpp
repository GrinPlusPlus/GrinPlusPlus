#include "MMRUtil.h"

#include <Common/Util/BitUtil.h>

//
// Calculates the postorder traversal index of all peaks in an MMR with the given size (# of nodes).
// Returns empty when the size does not represent a complete MMR (ie. siblings exist, but no parent).
//
std::vector<uint64_t> MMRUtil::GetPeakIndices(const uint64_t size)
{
	std::vector<uint64_t> peakIndices;
	if (size > 0) {
		uint64_t peakSize = BitUtil::FillOnesToRight(size);
		uint64_t numLeft = size;
		uint64_t sumPrevPeaks = 0;
		while (peakSize != 0) {
			if (numLeft >= peakSize) {
				peakIndices.push_back(sumPrevPeaks + peakSize - 1);
				sumPrevPeaks += peakSize;
				numLeft -= peakSize;
			}

			peakSize >>= 1;
		}

		if (numLeft > 0) {
			return std::vector<uint64_t>();
		}
	}

	return peakIndices;
}