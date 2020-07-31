#pragma once

#include <Core/Util/JsonUtil.h>
#include <Core/Traits/Jsonable.h>
#include <Core/Exceptions/APIException.h>
#include <Common/Secure.h>
#include <Common/GrinStr.h>
#include <optional>

class RestoreWalletCriteria : public Traits::IJsonable
{
public:
    RestoreWalletCriteria(
        const std::string& username,
        SecureString&& password,
        SecureString&& seedWords
    ) : m_username(username), m_password(std::move(password)), m_seedWords(std::move(seedWords)) { }
    virtual ~RestoreWalletCriteria() = default;

    const std::string& GetUsername() const noexcept { return m_username; }
    const SecureString& GetPassword() const noexcept { return m_password; }
    const SecureString& GetSeedWords() const noexcept { return m_seedWords; }

    static RestoreWalletCriteria FromJSON(const Json::Value& paramsJson)
    {
        if (paramsJson.isObject())
        {
            auto usernameOpt = JsonUtil::GetStringOpt(paramsJson, "username");
            auto passwordOpt = JsonUtil::GetStringOpt(paramsJson, "password");
            auto walletSeedOpt = JsonUtil::GetStringOpt(paramsJson, "wallet_seed");
            if (usernameOpt.has_value() && passwordOpt.has_value() && walletSeedOpt.has_value())
            {
                return RestoreWalletCriteria(
                    GrinStr{ usernameOpt.value() }.Trim().ToLower(),
                    SecureString(passwordOpt.value()),
                    SecureString(walletSeedOpt.value())
                );
            }
        }

        throw API_EXCEPTION(
            RPC::ErrorCode::INVALID_PARAMS,
            "Expected object with 3 parameters (username, password, wallet_seed)"
        );
    }

    Json::Value ToJSON() const final
    {
        Json::Value result;
        result["username"] = m_username;
        result["password"] = m_password.c_str();
        result["wallet_seed"] = m_seedWords.c_str();
        return result;
    }

private:
    std::string m_username;
    SecureString m_password;
    SecureString m_seedWords;
};