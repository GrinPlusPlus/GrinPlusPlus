#include "OwnerSend.h"

#include <Net/Tor/TorManager.h>
#include <Net/Tor/TorAddressParser.h>

OwnerSend::OwnerSend(const Config& config, IWalletManagerPtr pWalletManager)
	: m_config(config), m_pWalletManager(pWalletManager)
{

}

RPC::Response OwnerSend::Send(mg_connection* pConnection, RPC::Request& request)
{
	if (!request.GetParams().has_value())
	{
		throw DESERIALIZATION_EXCEPTION();
	}

	SendCriteria criteria = SendCriteria::FromJSON(request.GetParams().value());
	std::unique_ptr<TorConnection> pTorConnection = EstablishConnection(criteria.GetAddress());

	std::unique_ptr<Slate> pSlate = m_pWalletManager->Send(criteria);
	if (pSlate != nullptr)
	{
		if (pTorConnection != nullptr)
		{
			Json::Value params;
			params.append(pSlate->ToJSON());
			params.append(Json::nullValue); // Account path not currently supported
			params.append(criteria.GetMsg().has_value() ? criteria.GetMsg().value() : Json::Value(Json::nullValue));
			RPC::Request receiveTxRequest = RPC::Request::BuildRequest("receive_tx", params);
			RPC::Response receiveTxResponse = pTorConnection->Invoke(80, receiveTxRequest);

			if (receiveTxResponse.GetError().has_value())
			{
				const RPC::Error& error = receiveTxResponse.GetError().value();
				return RPC::Response::BuildError(request.GetId(), error.GetCode(), error.GetMsg(), error.GetData());
			}
			else
			{
				Json::Value okJson = JsonUtil::GetRequiredField(receiveTxResponse.GetResult().value(), "Ok");
				std::unique_ptr<Slate> pFinalizedSlate = m_pWalletManager->Finalize(criteria.GetToken(), Slate::FromJSON(okJson));
				if (pFinalizedSlate != nullptr)
				{
					m_pWalletManager->PostTransaction(criteria.GetToken(), pFinalizedSlate->GetTransaction());

					Json::Value result;
					result["status"] = "FINALIZED"; // TODO: Enum
					result["slate"] = pFinalizedSlate->ToJSON();
					return RPC::Response::BuildResult(request.GetId(), result);
				}
				else
				{
					Json::Value errorJson;
					errorJson["received_slate"] = okJson;
					return RPC::Response::BuildError(request.GetId(), RPC::ErrorCode::INTERNAL_ERROR, "Failed to finalize.", errorJson);
				}
			}
		}

		Json::Value result;
		result["slate"] = pSlate->ToJSON();
		return RPC::Response::BuildResult(request.GetId(), result);
	}
	else
	{
		return RPC::Response::BuildError(request.GetId(), RPC::ErrorCode::INTERNAL_ERROR, "Unknown error occurred.", std::nullopt);
	}
}

std::unique_ptr<TorConnection> OwnerSend::EstablishConnection(const std::optional<std::string>& addressOpt) const
{
	if (addressOpt.has_value())
	{
		const std::optional<TorAddress> torAddress = TorAddressParser::Parse(addressOpt.value());
		if (torAddress.has_value())
		{
			std::unique_ptr<TorConnection> pTorConnection = TorManager::GetInstance(m_config.GetTorConfig()).Connect(torAddress.value());
			if (pTorConnection == nullptr)
			{
				throw HTTP_EXCEPTION("Failed to establish TOR connection.");
			}

			if (!CheckVersion(*pTorConnection))
			{
				throw HTTP_EXCEPTION("Error occurred during check_version.");
			}

			return pTorConnection;
		}
	}

	return nullptr;
}

bool OwnerSend::CheckVersion(TorConnection& connection) const
{
	try
	{
		RPC::Request checkVersionRequest = RPC::Request::BuildRequest("check_version");
		const RPC::Response response = connection.Invoke(80, checkVersionRequest);
		if (response.GetError().has_value())
		{
			return false;
		}

		const Json::Value responseJSON = response.GetResult().value();
		const Json::Value okJSON = JsonUtil::GetRequiredField(responseJSON, "Ok");
		const uint64_t apiVersion = JsonUtil::GetRequiredUInt64(okJSON, "foreign_api_version");
		if (apiVersion != 2)
		{
			return false;
		}

		const Json::Value slateVersionsJSON = JsonUtil::GetRequiredField(okJSON, "supported_slate_versions");
		for (const Json::Value& slateVersion : slateVersionsJSON)
		{
			if (std::string(slateVersion.asCString()) == "V2")
			{
				return true;
			}
		}

		return true;
	}
	catch (const std::exception& e)
	{
		return false;
	}
}