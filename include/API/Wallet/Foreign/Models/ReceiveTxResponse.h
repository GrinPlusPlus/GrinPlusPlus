#pragma once

#include <Core/Util/JsonUtil.h>
#include <Core/Traits/Jsonable.h>
#include <Core/Exceptions/APIException.h>
#include <Wallet/Models/Slate/Slate.h>

class ReceiveTxResponse : public Traits::IJsonable
{
public:
    ReceiveTxResponse(Slate&& slate) : m_slate(std::move(slate)) { }
    ReceiveTxResponse(const Slate& slate) : m_slate(slate) { }
    virtual ~ReceiveTxResponse() = default;

    const Slate& GetSlate() const noexcept { return m_slate; }

    static ReceiveTxResponse FromJSON(const Json::Value& json)
    {
        Slate slate = Slate::FromJSON(JsonUtil::GetRequiredField(json, "Ok"));
        return ReceiveTxResponse(std::move(slate));
    }

    Json::Value ToJSON() const final
    {
        Json::Value result;
        result["Ok"] = m_slate.ToJSON();
        return result;
    }

private:
    Slate m_slate;
};