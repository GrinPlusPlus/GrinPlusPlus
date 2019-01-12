#pragma once

#include <Core/BlockHeader.h>

class Cuckatoo
{
public:
	static bool Validate(const BlockHeader& blockHeader);
};