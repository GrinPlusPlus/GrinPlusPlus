#pragma once

#include <Core/BlockHeader.h>

class Cuckaroo
{
public:
	static bool Validate(const BlockHeader& blockHeader);
};