#pragma once

#include <Core/Models/BlockHeader.h>

class Cuckarooz
{
public:
	static bool Validate(const BlockHeader& blockHeader);
};