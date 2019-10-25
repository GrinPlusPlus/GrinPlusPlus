#pragma once

#include <Common/Util/StringUtil.h>
#include <string>

namespace HTTP
{
enum class EHTTPMethod
{
    GET,
    POST
};

class Header
{
  public:
    Header(const std::string &type, const std::string &value) : m_type(type), m_value(value)
    {
    }

  private:
    std::string m_type;
    std::string m_value;
};

class Request
{
  public:
    Request(const EHTTPMethod method, const std::string &location, const std::string &host, const uint16_t port,
            const std::string &body)
        : m_method(method), m_location(location), m_host(host), m_port(port), m_body(body)
    {
    }

    std::string ToString() const
    {
        return StringUtil::Format("%s %s HTTP/1.1\r\n"
                                  "Host: %s\r\n"
                                  "Content-Length: %lu\r\n"
                                  "Content-Type: application/json-rpc\r\n\r\n%s",
                                  m_method == EHTTPMethod::GET ? "GET" : "POST", m_location.c_str(), m_host.c_str(),
                                  m_body.size(), m_body.c_str());
    }

    inline const std::string &GetHost() const
    {
        return m_host;
    }
    inline uint16_t GetPort() const
    {
        return m_port;
    }

  private:
    EHTTPMethod m_method;
    std::string m_location;
    std::string m_host;
    uint16_t m_port;
    std::string m_body;
};

class Response
{
  public:
    Response(const unsigned int statusCode, std::vector<Header> &&headers, const std::string &body)
        : m_statusCode(statusCode), m_headers(std::move(headers)), m_body(body)
    {
    }

    inline unsigned int GetStatusCode() const
    {
        return m_statusCode;
    }
    inline const std::vector<Header> &GetHeaders() const
    {
        return m_headers;
    }
    inline const std::string &GetBody() const
    {
        return m_body;
    }

  private:
    unsigned int m_statusCode;
    std::vector<Header> m_headers;
    std::string m_body;
};
} // namespace HTTP