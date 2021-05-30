#pragma once

#include <PMMR/Common/Index.h>
#include <PMMR/Common/LeafIndex.h>
#include <cstdint>
#include <vector>

class MMRUtil
{
public:
	static std::vector<uint64_t> GetPeakIndices(const uint64_t size);
};