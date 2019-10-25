#pragma once

#include <Net/Clients/HTTP/HTTP.h>
#include <Net/Clients/HTTP/HTTPClient.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Clients/RPC/RPCException.h>

class HttpRpcClient
{
  public:
    HttpRpcClient(IHTTPClient &httpClient) : m_httpClient(httpClient)
    {
    }

    RPC::Response Invoke(const std::string &host, const std::string &location, const uint16_t portNumber,
                         const RPC::Request &request)
    {
        try
        {
            const HTTP::Request httpRequest(HTTP::EHTTPMethod::POST, location, host, portNumber,
                                            JsonUtil::WriteCondensed(request.ToJSON()));
            const HTTP::Response response = m_httpClient.Invoke(httpRequest);

            return RPC::Response::Parse(response.GetBody());
        }
        catch (HTTPException &e)
        {
            throw RPC_EXCEPTION(e.what(), std::nullopt);
        }
    }

  private:
    IHTTPClient &m_httpClient;
};