#pragma once

#include <Core/Util/JsonUtil.h>
#include <Core/Traits/Jsonable.h>
#include <Core/Exceptions/APIException.h>
#include <Wallet/Models/Slate/Slate.h>

class FinalizeTxResponse : public Traits::IJsonable
{
public:
    FinalizeTxResponse(Slate&& slate) : m_slate(std::move(slate)) { }
    FinalizeTxResponse(const Slate& slate) : m_slate(slate) { }
    virtual ~FinalizeTxResponse() = default;

    const Slate& GetSlate() const noexcept { return m_slate; }

    static FinalizeTxResponse FromJSON(const Json::Value& json)
    {
        Slate slate = Slate::FromJSON(JsonUtil::GetRequiredField(json, "Ok"));
        return FinalizeTxResponse(std::move(slate));
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