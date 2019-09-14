#pragma once

#include <Core/Models/CompactBlock.h>
#include <Core/Models/FullBlock.h>

class CompactBlockFactory
{
public:
	static CompactBlock CreateCompactBlock(const FullBlock& block);
};