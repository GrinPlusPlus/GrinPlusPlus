#pragma once

#include <stdint.h>

struct ScryptParameters
{
	uint32_t N;
	uint32_t r;
	uint32_t p;

	ScryptParameters(const uint32_t N_, const uint32_t r_, const uint32_t p_)
		: N(N_), r(r_), p(p_)
	{

	}
};