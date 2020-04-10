#pragma once

#include <Core/Util/JsonUtil.h>
#include <Core/Traits/Jsonable.h>
#include <Core/Exceptions/APIException.h>
#include <Wallet/Models/Slate/Slate.h>
#include <optional>

class ReceiveTxRequest : public Traits::IJsonable
{
public:
    ReceiveTxRequest(
        const Slate& slate,
        const std::optional<std::string>& accountName,
        const std::optional<std::string>& message
    ) : m_slate(slate), m_accountName(accountName), m_message(message) { }
    virtual ~ReceiveTxRequest() = default;

    const Slate& GetSlate() const noexcept { return m_slate; }
    const std::optional<std::string>& GetAccountName() const noexcept { return m_accountName; }
    const std::optional<std::string>& GetMsg() const noexcept { return m_message; }

    static ReceiveTxRequest FromJSON(const Json::Value& paramsJson)
    {
        if (paramsJson.isArray() && paramsJson.size() == 3)
        {
            Slate slate = Slate::FromJSON(paramsJson[0]);
            std::optional<std::string> accountNameOpt = std::nullopt;
            if (!paramsJson[1].isNull())
            {
                accountNameOpt = std::make_optional(paramsJson[1].asString());
            }

            std::optional<std::string> messageOpt = std::nullopt;
            if (!paramsJson[2].isNull())
            {
                messageOpt = std::make_optional(paramsJson[2].asString());
            }

            return ReceiveTxRequest(slate, accountNameOpt, messageOpt);
        }

        throw API_EXCEPTION(
            RPC::ErrorCode::INVALID_PARAMS,
            "Expected 3 parameters (Slate, message, account_name)"
        );
    }

    Json::Value ToJSON() const final
    {
        Json::Value result;
        result.append(m_slate.ToJSON());
        result.append(m_accountName.has_value() ? Json::Value(m_accountName.value()) : Json::nullValue);
        result.append(m_message.has_value() ? Json::Value(m_message.value()) : Json::nullValue);
        return result;
    }

private:
    Slate m_slate;
    std::optional<std::string> m_accountName;
    std::optional<std::string> m_message;
};