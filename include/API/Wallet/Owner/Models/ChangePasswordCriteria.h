#pragma once

#include <Core/Util/JsonUtil.h>
#include <Core/Traits/Jsonable.h>
#include <Core/Exceptions/APIException.h>
#include <Common/Secure.h>
#include <optional>

class ChangePasswordCriteria : public Traits::IJsonable
{
public:
    ChangePasswordCriteria(
        const std::string& name,
        SecureString&& oldPassword,
        SecureString&& newPassword
    ) : m_name(name), m_oldPassword(std::move(oldPassword)), m_newPassword(std::move(newPassword)) { }

    ChangePasswordCriteria(
        const std::string& name,
        const SecureString& oldPassword,
        const SecureString& newPassword
    ) : m_name(name), m_oldPassword(oldPassword), m_newPassword(newPassword) { }

    virtual ~ChangePasswordCriteria() = default;

    const std::string& GetName() const noexcept { return m_name; }
    const SecureString& GetCurrentPassword() const noexcept { return m_oldPassword; }
    const SecureString& GetNewPassword() const noexcept { return m_newPassword; }

    static ChangePasswordCriteria FromJSON(const Json::Value& paramsJson)
    {
        if (paramsJson.isObject())
        {
            auto nameOpt = JsonUtil::GetStringOpt(paramsJson, "name");
            auto oldPasswordOpt = JsonUtil::GetStringOpt(paramsJson, "old");
            auto newPasswordOpt = JsonUtil::GetStringOpt(paramsJson, "new");

            if (nameOpt.has_value() && oldPasswordOpt.has_value() && newPasswordOpt.has_value())
            {
                return ChangePasswordCriteria(
                    StringUtil::ToLower(nameOpt.value()),
                    SecureString(oldPasswordOpt.value()),
                    SecureString(newPasswordOpt.value())
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
        result["name"] = m_name;
        result["old"] = m_oldPassword.c_str();
        result["new"] = m_newPassword.c_str();
        return result;
    }

private:
    std::string m_name;
    SecureString m_oldPassword;
    SecureString m_newPassword;
};