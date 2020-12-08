#pragma once

enum class EProtocolVersion
{
    V1 = 1,
    V2 = 2,
    V3 = 3,
    V1000 = 1000
};

namespace ProtocolVersion
{
    static constexpr EProtocolVersion Local()
    {
        return EProtocolVersion::V1000;
    }

    static EProtocolVersion ToEnum(const uint32_t version)
    {
        if (version >= 1000) {
            return EProtocolVersion::V1000;
        } else if (version >= 3) {
            return EProtocolVersion::V3;
        } else if (version == 2) {
            return EProtocolVersion::V2;
        } else {
            return EProtocolVersion::V1;
        }
    }
}