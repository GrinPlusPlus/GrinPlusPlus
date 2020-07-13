#pragma once

#include <Core/Util/JsonUtil.h>
#include <optional>

struct SlateFeatureArgs
{
    std::optional<uint64_t> lockHeightOpt;

    Json::Value ToJSON() const noexcept
    {
        Json::Value json;

        if (lockHeightOpt.has_value()) {
            json["lock_hgt"] = lockHeightOpt.value();
        }

        return json;
    }

    SlateFeatureArgs FromJSON(const Json::Value& json)
    {
        SlateFeatureArgs args;
        args.lockHeightOpt = JsonUtil::GetUInt64Opt(json, "lock_hgt");
        return args;
    }
};