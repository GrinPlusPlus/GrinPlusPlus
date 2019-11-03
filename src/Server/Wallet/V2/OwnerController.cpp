#include "OwnerController.h"

#include <Net/Clients/RPC/RPC.h>
#include <Net/Tor/TorManager.h>
#include <Net/Tor/TorAddressParser.h>
#include <Net/Tor/TorException.h>
#include <Core/Exceptions/DeserializationException.h>
#include <Net/Clients/HTTP/HTTPException.h>

#include "Send/OwnerSend.h"

OwnerController::OwnerController(std::shared_ptr<CallbackData> pCallbackData, mg_context* pCivetContext)
	: m_pCallbackData(pCallbackData), m_pCivetContext(pCivetContext)
{

}

OwnerController::~OwnerController()
{
	mg_stop(m_pCivetContext);
}

std::shared_ptr<OwnerController> OwnerController::Create(const Config& config, IWalletManagerPtr pWalletManager)
{
	std::shared_ptr<CallbackData> pCallbackData(new CallbackData(config, pWalletManager));

	const char* pOptions[] = {
		"num_threads", "1",
		"listening_ports", "3421", // TODO: Figure out which port to use
		NULL
	};

	mg_context* pCivetContext = mg_start(NULL, 0, pOptions);
	mg_set_request_handler(pCivetContext, "/v2", OwnerAPIHandler, pCallbackData.get());

	return std::shared_ptr<OwnerController>(new OwnerController(pCallbackData, pCivetContext));
}

int OwnerController::OwnerAPIHandler(mg_connection* pConnection, void* pCBData)
{
	Json::Value responseJSON;
	try
	{
		RPC::Request request = RPC::Request::Parse(pConnection);

		try
		{
			const std::string& method = request.GetMethod();
			if (method == "send")
			{
				CallbackData* pCallbackData = (CallbackData*)pCBData;
				OwnerSend send(pCallbackData->m_config, pCallbackData->m_pWalletManager);

				responseJSON = send.Send(pConnection, request).ToJSON();
			}
			else
			{
				responseJSON = RPC::Response::BuildError(request.GetId(), RPC::ErrorCode::METHOD_NOT_FOUND, "Method not supported", std::nullopt).ToJSON();
			}
		}
		catch (const DeserializationException& e)
		{
			responseJSON = RPC::Response::BuildError(request.GetId(), RPC::ErrorCode::INVALID_REQUEST, e.GetMsg(), std::nullopt).ToJSON();
		}
		catch (const HTTPException& e)
		{
			responseJSON = RPC::Response::BuildError(request.GetId(), RPC::ErrorCode::INTERNAL_ERROR, e.GetMsg(), std::nullopt).ToJSON();
		}
		catch (const TorException& e)
		{
			responseJSON = RPC::Response::BuildError(request.GetId(), RPC::ErrorCode::INTERNAL_ERROR, e.GetMsg(), std::nullopt).ToJSON();
		}
	}
	catch (const RPCException& e)
	{
		responseJSON = RPC::Response::BuildError(e.GetId().value_or(Json::nullValue), RPC::ErrorCode::INVALID_REQUEST, e.what(), std::nullopt).ToJSON();
	}

	return HTTPUtil::BuildSuccessResponseJSON(pConnection, responseJSON);
}