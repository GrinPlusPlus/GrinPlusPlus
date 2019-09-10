#pragma once

#include "MMR.h"
#include <Core/CRoaring/roaring.hh>

class PruneableMMR : public MMR
{
public:
	virtual bool Rewind(const uint64_t size, const Roaring& leavesToAdd) = 0;
};