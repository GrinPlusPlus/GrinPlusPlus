#pragma once

#include <Core/Traits/Jsonable.h>
#include <Wallet/Models/Slate/Slate.h>
#include <string>

class SendResponse : public Traits::IJsonable
{
public:
    enum class EStatus
    {
        SENT,
        FINALIZED
    };

    SendResponse(
        const EStatus status,
        const Slate& slate,
        const std::string& armoredSlate,
        const std::string& error = ""
    ) : m_status(status), m_slate(slate), m_armoredSlate(armoredSlate), m_error(error) { }

    EStatus GetStatus() const noexcept { return m_status; }
    const Slate& GetSlate() const noexcept { return m_slate; }
    const std::string& GetArmoredSlate() const noexcept { return m_armoredSlate; }
    const std::string& GetError() const noexcept { return m_error; }

    Json::Value ToJSON() const noexcept final
    {
        Json::Value result;
        result["status"] = m_status == EStatus::SENT ? "SENT" : "FINALIZED";
        result["slate"] = m_slate.ToJSON();
        result["slatepack"] = m_armoredSlate;

        if (!m_error.empty())
        {
            result["error"] = m_error;
        }

        return result;
    }

    static SendResponse FromJSON(const Json::Value& json)
    {
        std::string statusStr = JsonUtil::GetRequiredString(json, "status");
        EStatus status = EStatus::SENT;
        if (statusStr == "SENT") {
            status = EStatus::SENT;
        } else if (statusStr == "FINALIZED") {
            status = EStatus::FINALIZED;
        } else {
            throw std::exception();
        }

        Slate slate = Slate::FromJSON(JsonUtil::GetRequiredField(json, "slate"));
        std::string armoredSlate = JsonUtil::GetRequiredString(json, "slatepack");
        std::string error = JsonUtil::GetStringOpt(json, "error").value_or("");

        return SendResponse(status, slate, armoredSlate, error);
    }

private:
    EStatus m_status;
    Slate m_slate;
    std::string m_armoredSlate;
    std::string m_error;
};