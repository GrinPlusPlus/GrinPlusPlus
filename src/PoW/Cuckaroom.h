#pragma once

#include <Core/Models/BlockHeader.h>

class Cuckaroom
{
public:
	static bool Validate(const BlockHeader& blockHeader);
};