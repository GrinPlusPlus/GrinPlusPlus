#pragma once

#include <exception>
#include <json/json.h>
#include <optional>

#define RPC_EXCEPTION(msg, id) RPCException(__FUNCTION__, msg, id)

class RPCException : public std::exception
{
  public:
    RPCException(const std::string &function, const std::string &message, const std::optional<Json::Value> &idOpt)
        : std::exception()
    {
        m_message = message;
        m_idOpt = idOpt;
        m_what = function + " - " + message;
    }

    RPCException() : std::exception()
    {
        m_message = "RPC Exception";
    }

    virtual const char *what() const throw()
    {
        return m_what.c_str();
    }

    inline const std::optional<Json::Value> &GetId() const
    {
        return m_idOpt;
    }

  private:
    std::string m_message;
    std::string m_what;
    std::optional<Json::Value> m_idOpt;
};