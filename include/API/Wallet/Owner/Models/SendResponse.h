#pragma once

#include <Core/Traits/Jsonable.h>
#include <string>

class SendResponse : public Traits::IJsonable
{
public:
    enum EStatus
    {
        SENT,
        FINALIZED
    };

    SendResponse(
        const EStatus status,
        const std::string& armoredSlate,
        const std::string& error = ""
    ) : m_status(status), m_armoredSlate(armoredSlate), m_error(error) { }

    EStatus GetStatus() const noexcept { return m_status; }
    const std::string& GetArmoredSlate() const noexcept { return m_armoredSlate; }
    const std::string& GetError() const noexcept { return m_error; }

    Json::Value ToJSON() const noexcept final
    {
        Json::Value result;
        result["status"] = m_status == EStatus::SENT ? "SENT" : "FINALIZED";
        result["slatepack"] = m_armoredSlate;

        if (!m_error.empty())
        {
            result["error"] = m_error;
        }

        return result;
    }

private:
    EStatus m_status;
    std::string m_armoredSlate;
    std::string m_error;
};