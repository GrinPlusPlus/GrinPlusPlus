#pragma once

#include <Core/Util/JsonUtil.h>
#include <Core/Traits/Jsonable.h>
#include <Core/Exceptions/APIException.h>
#include <Common/Secure.h>
#include <optional>

class CreateWalletCriteria : public Traits::IJsonable
{
public:
    CreateWalletCriteria(
        const std::string& username,
        SecureString&& password,
        const int numWords
    ) : m_username(username), m_password(std::move(password)), m_numWords(numWords) { }

    CreateWalletCriteria(
        const std::string& username,
        const SecureString& password,
        const int numWords
    ) : m_username(username), m_password(password), m_numWords(numWords) { }

    virtual ~CreateWalletCriteria() = default;

    const std::string& GetUsername() const noexcept { return m_username; }
    const SecureString& GetPassword() const noexcept { return m_password; }
    int GetNumWords() const noexcept { return m_numWords; }

    static CreateWalletCriteria FromJSON(const Json::Value& paramsJson)
    {
        if (paramsJson.isObject())
        {
            auto usernameOpt = JsonUtil::GetStringOpt(paramsJson, "username");
            auto passwordOpt = JsonUtil::GetStringOpt(paramsJson, "password");
            auto numWordsOpt = JsonUtil::GetUInt64Opt(paramsJson, "num_seed_words");
            if (usernameOpt.has_value() && passwordOpt.has_value() && numWordsOpt.has_value())
            {
                return CreateWalletCriteria(
                    StringUtil::Trim(StringUtil::ToLower(*usernameOpt)),
                    SecureString(*passwordOpt),
                    (int)*numWordsOpt
                );
            }
        }

        throw API_EXCEPTION(
            RPC::ErrorCode::INVALID_PARAMS,
            "Expected object with 3 parameters (username, password, num_seed_words)"
        );
    }

    Json::Value ToJSON() const final
    {
        Json::Value result;
        result["username"] = m_username;
        result["password"] = m_password.c_str();
        result["num_seed_words"] = m_numWords;
        return result;
    }

private:
    std::string m_username;
    SecureString m_password;
    int m_numWords;
};