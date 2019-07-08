#pragma once

#include <stdint.h>
#include <Core/Util/JsonUtil.h>

struct ScryptParameters
{
	uint32_t N;
	uint32_t r;
	uint32_t p;

	ScryptParameters(const uint32_t N_, const uint32_t r_, const uint32_t p_)
		: N(N_), r(r_), p(p_)
	{

	}

	Json::Value ToJSON() const
	{
		Json::Value scryptJSON;
		scryptJSON["N"] = N;
		scryptJSON["r"] = r;
		scryptJSON["p"] = p;
		return scryptJSON;
	}

	static ScryptParameters FromJSON(const Json::Value& scryptJSON)
	{
		const uint32_t N_ = JsonUtil::GetRequiredField(scryptJSON, "N").asUInt();
		const uint32_t r_ = JsonUtil::GetRequiredField(scryptJSON, "r").asUInt();
		const uint32_t p_ = JsonUtil::GetRequiredField(scryptJSON, "p").asUInt();
		return ScryptParameters(N_, r_, p_);
	}
};