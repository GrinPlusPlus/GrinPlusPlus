#include "OwnerController.h"

#include <Core/Serialization/DeserializationException.h>
#include <Net/Clients/HTTP/HTTPException.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Tor/TorAddressParser.h>
#include <Net/Tor/TorException.h>
#include <Net/Tor/TorManager.h>

#include "Send/OwnerSend.h"

OwnerController::OwnerController(const Config &config) : m_config(config)
{
}

bool OwnerController::StartListener(IWalletManager *pWalletManager)
{
    m_pWalletManager = pWalletManager;

    const char *pOptions[] = {"num_threads", "1", "listening_ports", "3421", // TODO: Figure out which port to use
                              NULL};

    m_pCivetContext = mg_start(NULL, 0, pOptions);
    if (m_pCivetContext == nullptr)
    {
        return false;
    }

    mg_set_request_handler(m_pCivetContext, "/v2", OwnerAPIHandler, this);
    return true;
}

bool OwnerController::StopListener()
{
    mg_stop(m_pCivetContext);
    return true;
}

int OwnerController::OwnerAPIHandler(mg_connection *pConnection, void *pContext)
{
    Json::Value responseJSON;
    try
    {
        RPC::Request request = RPC::Request::Parse(pConnection);

        try
        {
            const std::string &method = request.GetMethod();
            if (method == "send")
            {
                OwnerController *pController = (OwnerController *)pContext;
                OwnerSend send(pController->m_config, *pController->m_pWalletManager);

                responseJSON = send.Send(pConnection, request).ToJSON();
            }
            else
            {
                responseJSON = RPC::Response::BuildError(request.GetId(), RPC::ErrorCode::METHOD_NOT_FOUND,
                                                         "Method not supported", std::nullopt)
                                   .ToJSON();
            }
        }
        catch (const DeserializationException &e)
        {
            responseJSON =
                RPC::Response::BuildError(request.GetId(), RPC::ErrorCode::INVALID_REQUEST, e.GetMsg(), std::nullopt)
                    .ToJSON();
        }
        catch (const HTTPException &e)
        {
            responseJSON =
                RPC::Response::BuildError(request.GetId(), RPC::ErrorCode::INTERNAL_ERROR, e.GetMsg(), std::nullopt)
                    .ToJSON();
        }
        catch (const TorException &e)
        {
            responseJSON =
                RPC::Response::BuildError(request.GetId(), RPC::ErrorCode::INTERNAL_ERROR, e.GetMsg(), std::nullopt)
                    .ToJSON();
        }
    }
    catch (const RPCException &e)
    {
        responseJSON = RPC::Response::BuildError(e.GetId().value_or(Json::nullValue), RPC::ErrorCode::INVALID_REQUEST,
                                                 e.what(), std::nullopt)
                           .ToJSON();
    }

    return HTTPUtil::BuildSuccessResponseJSON(pConnection, responseJSON);
}