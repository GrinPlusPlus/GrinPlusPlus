#pragma once

#include <Core/Models/BlockHeader.h>

class Cuckatoo
{
public:
	static bool Validate(const BlockHeader& blockHeader);
};