#pragma once

#include <Core/Models/BlockHeader.h>

class Cuckarood
{
public:
	static bool Validate(const BlockHeader& blockHeader);
};