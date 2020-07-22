#pragma once

#include <Core/Util/JsonUtil.h>
#include <Core/Traits/Jsonable.h>
#include <Core/Exceptions/APIException.h>
#include <Wallet/Models/Slate/Slate.h>
#include <optional>

class FinalizeTxRequest : public Traits::IJsonable
{
public:
    FinalizeTxRequest(const Slate& slate)
        : m_slate(slate) { }
    virtual ~FinalizeTxRequest() = default;

    const Slate& GetSlate() const noexcept { return m_slate; }

    static FinalizeTxRequest FromJSON(const Json::Value& paramsJson)
    {
        if (paramsJson.isArray() && paramsJson.size() >= 1)
        {
            Slate slate = Slate::FromJSON(paramsJson[0]);

            return FinalizeTxRequest(slate);
        }

        throw API_EXCEPTION(
            RPC::ErrorCode::INVALID_PARAMS,
            "Expected 1 parameter (Slate)"
        );
    }

    Json::Value ToJSON() const final
    {
        Json::Value result;
        result.append(m_slate.ToJSON());
        return result;
    }

private:
    Slate m_slate;
};