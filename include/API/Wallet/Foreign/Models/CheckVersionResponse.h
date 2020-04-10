#pragma once

#include <Core/Util/JsonUtil.h>
#include <Core/Traits/Jsonable.h>
#include <Core/Exceptions/APIException.h>

class CheckVersionResponse : public Traits::IJsonable
{
public:
    CheckVersionResponse(const uint64_t foreignApiVersion, const std::vector<uint64_t>& supportedSlateVersions)
        : m_foreignApiVersion(foreignApiVersion), m_supportedSlateVersions(supportedSlateVersions) { }
    virtual ~CheckVersionResponse() = default;

    uint64_t GetForeignApiVersion() const noexcept { return m_foreignApiVersion; }
    const std::vector<uint64_t>& GetSupportedSlateVersions() const noexcept { return m_supportedSlateVersions; }

    static CheckVersionResponse FromJSON(const Json::Value& json)
    {
        Json::Value okJson = JsonUtil::GetRequiredField(json, "Ok");

        const uint64_t foreignApiVersion = JsonUtil::GetRequiredUInt64(okJson, "foreign_api_version");

        Json::Value slateVersionsJson = JsonUtil::GetRequiredField(okJson, "supported_slate_versions");
        if (!slateVersionsJson.isArray())
        {
            throw API_EXCEPTION(RPC::ErrorCode::INVALID_PARAMS, "supported_slate_versions must be an array.");
        }

        std::vector<uint64_t> slateVersions;
        for (const Json::Value& slateVersion : slateVersionsJson)
        {
            if (!slateVersion.isString())
            {
                throw API_EXCEPTION(RPC::ErrorCode::INVALID_PARAMS, "Slate version must be a string.");
            }

            std::string slateVersionStr = slateVersion.asString();
            if (slateVersionStr.size() < 2 || slateVersionStr[0] != 'V')
            {
                throw API_EXCEPTION_F(RPC::ErrorCode::INVALID_PARAMS, "Slate version {} is invalid.", slateVersionStr);
            }

            try
            {
                slateVersions.push_back(std::stoull(slateVersionStr.substr(1)));
            }
            catch (const std::invalid_argument&)
            {
                throw API_EXCEPTION_F(RPC::ErrorCode::INVALID_PARAMS, "Slate version {} is invalid.", slateVersionStr);
            }
        }

        return CheckVersionResponse(foreignApiVersion, slateVersions);
    }

    Json::Value ToJSON() const final
    {
        Json::Value versions;
        versions["foreign_api_version"] = m_foreignApiVersion;

        Json::Value slateVersions;
        for (const uint64_t version : m_supportedSlateVersions)
        {
            slateVersions.append("V" + std::to_string(version));
        }

        versions["supported_slate_versions"] = slateVersions;

        Json::Value result;
        result["Ok"] = versions;

        return result;
    }

private:
    uint64_t m_foreignApiVersion;
    std::vector<uint64_t> m_supportedSlateVersions;
};