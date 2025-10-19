#pragma once

#include <Core/Util/JsonUtil.h>
#include <Core/Traits/Jsonable.h>
#include <Core/Exceptions/APIException.h>
#include <Common/Secure.h>
#include <optional>

class LoginCriteria : public Traits::IJsonable
{
public:
    LoginCriteria(
        const std::string& username,
        SecureString&& password
    ) : m_username(username), m_password(std::move(password)) { }

    LoginCriteria(
        const std::string& username,
        const SecureString& password
    ) : m_username(username), m_password(password) { }

    virtual ~LoginCriteria() = default;

    const std::string& GetUsername() const noexcept { return m_username; }
    const SecureString& GetPassword() const noexcept { return m_password; }

    static LoginCriteria FromJSON(const Json::Value& paramsJson)
    {
        if (paramsJson.isObject())
        {
            auto usernameOpt = JsonUtil::GetStringOpt(paramsJson, "name");
            auto passwordOpt = JsonUtil::GetStringOpt(paramsJson, "password");
            if (usernameOpt.has_value() && passwordOpt.has_value())
            {
                return LoginCriteria(
                    StringUtil::ToLower(usernameOpt.value()),
                    SecureString(passwordOpt.value())
                );
            }
        }

        throw API_EXCEPTION(
            RPC::ErrorCode::INVALID_PARAMS,
            "Expected object with 2 parameters (name, password)"
        );
    }

    Json::Value ToJSON() const final
    {
        Json::Value result;
        result["name"] = m_username;
        result["password"] = m_password.c_str();
        return result;
    }

private:
    std::string m_username;
    SecureString m_password;
};