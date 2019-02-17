#include "MMRUtil.h"

#include <Core/Serialization/Serializer.h>
#include <Crypto/Crypto.h>
#include <Common/Util/BitUtil.h>
#include <vector>

//
// Calculates the height of the node at the given position (mmrIndex).
// mmrIndex is the zero-based postorder traversal index of a node in the tree.
//
// Height      Index
//
// 2:            6
//              / \
//             /   \
// 1:         2     5
//           / \   / \
// 0:       0   1 3   4
//
uint64_t MMRUtil::GetHeight(const uint64_t mmrIndex)
{
	uint64_t height = mmrIndex;
	uint64_t peakSize = BitUtil::FillOnesToRight(mmrIndex + 1);
	while (peakSize != 0)
	{
		if (height >= peakSize)
		{
			height -= peakSize;
		}

		peakSize >>= 1;
	}

	return height;
}

// NOTE: This is not the fastest way to do this, but it's plenty fast for now
uint64_t MMRUtil::GetParentIndex(const uint64_t mmrIndex)
{
	const uint64_t height = GetHeight(mmrIndex);

	if (GetHeight(mmrIndex + 1) == (height + 1))
	{
		// mmrIndex points to a right sibling, so the next node is the parent.
		return mmrIndex + 1;
	}
	else
	{
		// mmrIndex is the left sibling, so the parent node is mmrIndex + 2^(height + 1).
		return mmrIndex + (1 << (height + 1));
	}
}

// NOTE: This is not the fastest way to do this, but it's plenty fast for now
uint64_t MMRUtil::GetSiblingIndex(const uint64_t mmrIndex)
{
	const uint64_t height = GetHeight(mmrIndex);

	if (GetHeight(mmrIndex + 1) == (height + 1))
	{
		// mmrIndex points to a right sibling, so add 1 and subtract 2^(height + 1) to get the left sibling.
		return mmrIndex + 1 - (1 << (height + 1));
	}
	else
	{
		// mmrIndex is the left sibling, so add 2^(height + 1) - 1 to get the right sibling.
		return mmrIndex + (1 << (height + 1)) - 1;
	}
}

// WARNING: Assumes mmrIndex is a parent.
uint64_t MMRUtil::GetLeftChildIndex(const uint64_t mmrIndex, const uint64_t height)
{
	return mmrIndex - (1 << height);
}

// WARNING: Assumes mmrIndex is a parent.
uint64_t MMRUtil::GetRightChildIndex(const uint64_t mmrIndex)
{
	return mmrIndex - 1;
}

//
// Calculates the postorder traversal index of all peaks in an MMR with the given size (# of nodes).
// Returns empty when the size does not represent a complete MMR (ie. siblings exist, but no parent).
//
std::vector<uint64_t> MMRUtil::GetPeakIndices(const uint64_t size)
{
	std::vector<uint64_t> peakIndices;
	if (size > 0)
	{
		uint64_t peakSize = BitUtil::FillOnesToRight(size);
		uint64_t numLeft = size;
		uint64_t sumPrevPeaks = 0;
		while (peakSize != 0)
		{
			if (numLeft >= peakSize)
			{
				peakIndices.push_back(sumPrevPeaks + peakSize - 1);
				sumPrevPeaks += peakSize;
				numLeft -= peakSize;
			}

			peakSize >>= 1;
		}

		if (numLeft > 0)
		{
			return std::vector<uint64_t>();
		}
	}

	return peakIndices;
}

std::vector<uint64_t> MMRUtil::GetPeakSizes(const uint64_t size)
{
	std::vector<uint64_t> peakSizes;
	if (size > 0)
	{
		uint64_t peakSize = BitUtil::FillOnesToRight(size);
		uint64_t numLeft = size;
		while (peakSize != 0)
		{
			if (numLeft >= peakSize)
			{
				peakSizes.push_back(peakSize);
				numLeft -= peakSize;
			}

			peakSize >>= 1;
		}
	}

	return peakSizes;
}

// TODO: Faster method likely exists
uint64_t MMRUtil::GetNumNodes(const uint64_t pmmrIndex)
{
	uint64_t numNodes = pmmrIndex;
	uint64_t height = GetHeight(numNodes);
	uint64_t nextNodeHeight = GetHeight(++numNodes);
	while (nextNodeHeight > height)
	{
		height = nextNodeHeight;
		nextNodeHeight = GetHeight(++numNodes);
	}

	return numNodes;
}

uint64_t MMRUtil::GetNumLeaves(const uint64_t lastMMRIndex)
{
	const uint64_t numNodes = GetNumNodes(lastMMRIndex);

	uint64_t numLeaves = 0;

	std::vector<uint64_t> peakSizes = GetPeakSizes(numNodes);
	for (uint64_t peakSize : peakSizes)
	{
		numLeaves += ((peakSize + 1)/ 2);
	}

	return numLeaves;
}

bool MMRUtil::IsLeaf(const uint64_t mmrIndex)
{
	return GetHeight(mmrIndex) == 0;
}

uint64_t MMRUtil::GetPMMRIndex(const uint64_t leafIndex)
{
	return 2 * leafIndex - BitUtil::CountBitsSet(leafIndex);
}