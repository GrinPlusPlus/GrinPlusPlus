#pragma once

#include <Core/Models/BlockHeader.h>

class Cuckaroo
{
public:
	static bool Validate(const BlockHeader& blockHeader);
};