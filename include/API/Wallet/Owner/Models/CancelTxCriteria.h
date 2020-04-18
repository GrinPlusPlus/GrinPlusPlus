#pragma once

#include <API/Wallet/Owner/Models/Errors.h>
#include <Core/Util/JsonUtil.h>
#include <Core/Traits/Jsonable.h>
#include <Core/Exceptions/APIException.h>
#include <Wallet/SessionToken.h>
#include <optional>

class CancelTxCriteria : public Traits::IJsonable
{
public:
    CancelTxCriteria(const SessionToken& token, const uint32_t txId)
        : m_token(token), m_txId(txId) { }
    virtual ~CancelTxCriteria() = default;

    const SessionToken& GetToken() const noexcept { return m_token; }
    uint32_t GetTxId() const noexcept { return m_txId; }

    static CancelTxCriteria FromJSON(const Json::Value& paramsJson)
    {
        if (paramsJson.isObject())
        {
            SessionToken token = SessionToken::FromBase64(
                JsonUtil::GetRequiredString(paramsJson, "session_token")
            );

            const Json::Value& txIdJson = paramsJson["tx_id"];
            if (txIdJson == Json::nullValue)
            {
                throw API_EXCEPTION(
                    RPC::Errors::TX_ID_MISSING.GetCode(),
                    "'tx_id' missing"
                );
            }

            const uint32_t tx_id = (uint32_t)JsonUtil::ConvertToUInt64(txIdJson);

            return CancelTxCriteria(token, tx_id);
        }

        throw API_EXCEPTION(
            RPC::ErrorCode::INVALID_PARAMS,
            "Expected object with 2 parameters (session_token, tx_id)"
        );
    }

    Json::Value ToJSON() const final
    {
        Json::Value result;
        result["session_token"] = m_token.ToBase64();
        result["tx_id"] = m_txId;
        return result;
    }

private:
    SessionToken m_token;
    uint32_t m_txId;
};