#pragma once

#include <Core/Util/JsonUtil.h>
#include <Core/Traits/Jsonable.h>
#include <Core/Exceptions/APIException.h>
#include <Common/Secure.h>
#include <optional>

class GetSeedPhraseCriteria : public Traits::IJsonable
{
public:
    GetSeedPhraseCriteria(
        const std::string& username,
        SecureString&& password
    ) : m_username(username), m_password(std::move(password)) { }

    GetSeedPhraseCriteria(
        const std::string& username,
        const SecureString& password
    ) : m_username(username), m_password(password) { }

    virtual ~GetSeedPhraseCriteria() = default;

    const std::string& GetUsername() const noexcept { return m_username; }
    const SecureString& GetPassword() const noexcept { return m_password; }

    static GetSeedPhraseCriteria FromJSON(const Json::Value& paramsJson)
    {
        if (paramsJson.isObject())
        {
            auto usernameOpt = JsonUtil::GetStringOpt(paramsJson, "username");
            auto passwordOpt = JsonUtil::GetStringOpt(paramsJson, "password");
            if (usernameOpt.has_value() && passwordOpt.has_value())
            {
                return GetSeedPhraseCriteria(
                    StringUtil::ToLower(usernameOpt.value()),
                    SecureString(passwordOpt.value())
                );
            }
        }

        throw API_EXCEPTION(
            RPC::ErrorCode::INVALID_PARAMS,
            "Expected object with 2 parameters (username, password)"
        );
    }

    Json::Value ToJSON() const final
    {
        Json::Value result;
        result["username"] = m_username;
        result["password"] = m_password.c_str();
        return result;
    }

private:
    std::string m_username;
    SecureString m_password;
};