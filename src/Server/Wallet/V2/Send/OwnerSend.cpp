#include "OwnerSend.h"

#include <Net/Tor/TorManager.h>
#include <Net/Tor/TorAddressParser.h>
#include <Common/Util/FileUtil.h>

OwnerSend::OwnerSend(const Config& config, IWalletManagerPtr pWalletManager)
	: m_config(config), m_pWalletManager(pWalletManager)
{

}

RPC::Response OwnerSend::Handle(const RPC::Request& request) const
{
	if (!request.GetParams().has_value())
	{
		throw DESERIALIZATION_EXCEPTION();
	}

	SendCriteria criteria = SendCriteria::FromJSON(request.GetParams().value());

	const std::optional<TorAddress> torAddress = criteria.GetAddress().has_value() ?
		TorAddressParser::Parse(criteria.GetAddress().value()) : std::nullopt;

	if (torAddress.has_value())
	{
		return SendViaTOR(request, criteria, torAddress.value());
	}
	else if (criteria.GetFile().has_value())
	{
		return SendViaFile(request, criteria, criteria.GetFile().value());
	}
	else
	{
		Slate slate = m_pWalletManager->Send(criteria);

		Json::Value result;
		result["status"] = "SENT";
		result["slate"] = slate.ToJSON();
		return request.BuildResult(result);
	}
}

RPC::Response OwnerSend::SendViaTOR(const RPC::Request& request, SendCriteria& criteria, const TorAddress& torAddress) const
{
	TorConnectionPtr pTorConnection = TorManager::GetInstance(m_config.GetTorConfig()).Connect(torAddress);
	if (pTorConnection == nullptr)
	{
		throw HTTP_EXCEPTION("Failed to establish TOR connection.");
	}

	const uint16_t version = CheckVersion(*pTorConnection);
	if (version < MIN_SLATE_VERSION)
	{
		throw HTTP_EXCEPTION("Slate version incompatibility.");
	}

	criteria.SetSlateVersion(version);
	Slate slate = m_pWalletManager->Send(criteria);

	Json::Value params;
	params.append(slate.ToJSON());
	params.append(Json::nullValue); // Account path not currently supported
	params.append(criteria.GetMsg().has_value() ? criteria.GetMsg().value() : Json::Value(Json::nullValue));
	RPC::Request receiveTxRequest = RPC::Request::BuildRequest("receive_tx", params);
	std::string result = receiveTxRequest.ToJSON().toStyledString();
	LOG_ERROR_F("{}", result);
	RPC::Response receiveTxResponse = pTorConnection->Invoke(receiveTxRequest, "/v2/foreign");

	if (receiveTxResponse.GetError().has_value())
	{
		const RPC::Error& error = receiveTxResponse.GetError().value();
		return request.BuildError(error.GetCode(), error.GetMsg(), error.GetData());
	}
	else
	{
		result = receiveTxResponse.GetResult().value().toStyledString();
		LOG_ERROR_F("{}", result);
		Json::Value okJson = JsonUtil::GetRequiredField(receiveTxResponse.GetResult().value(), "Ok");
		try
		{
			Slate finalizedSlate = m_pWalletManager->Finalize(FinalizeCriteria(SessionToken(criteria.GetToken()), Slate::FromJSON(okJson), std::nullopt, criteria.GetPostMethod()));

			Json::Value resultJSON;
			resultJSON["status"] = "FINALIZED"; // TODO: Enum
			resultJSON["slate"] = finalizedSlate.ToJSON();
			return RPC::Response::BuildResult(request.GetId(), resultJSON);
		}
		catch (std::exception&)
		{
			Json::Value errorJson;
			errorJson["status"] = "FINALIZE_FAILED";
			errorJson["slate"] = okJson;
			return request.BuildError(RPC::ErrorCode::INTERNAL_ERROR, "Failed to finalize.", errorJson);
		}
	}
}

RPC::Response OwnerSend::SendViaFile(const RPC::Request& request, const SendCriteria& criteria, const std::string& file) const
{
	// FUTURE: Check write permissions before creating slate

	Slate slate = m_pWalletManager->Send(criteria);

	Json::Value slateJSON = slate.ToJSON();
	if (FileUtil::WriteTextToFile(file, JsonUtil::WriteCondensed(slateJSON)))
	{
		Json::Value result;
		result["status"] = "SENT";
		result["slate"] = slateJSON;
		return RPC::Response::BuildResult(request.GetId(), result);
	}
	else
	{
		Json::Value errorJson;
		errorJson["status"] = "WRITE_FAILED";
		errorJson["slate"] = slateJSON;
		return request.BuildError(RPC::ErrorCode::INTERNAL_ERROR, "Failed to write file.", errorJson);
	}
}

uint16_t OwnerSend::CheckVersion(TorConnection& connection) const
{
	try
	{
		RPC::Request checkVersionRequest = RPC::Request::BuildRequest("check_version");
		const RPC::Response response = connection.Invoke(checkVersionRequest, "/v2/foreign");
		if (response.GetError().has_value())
		{
			LOG_ERROR_F("check_version failed with error: {}", response.GetError().value().GetMsg());
			throw HTTP_EXCEPTION("check_version call failed");
		}

		const Json::Value responseJSON = response.GetResult().value();
		const Json::Value okJSON = JsonUtil::GetRequiredField(responseJSON, "Ok");
		const uint64_t apiVersion = JsonUtil::GetRequiredUInt64(okJSON, "foreign_api_version");
		if (apiVersion != 2)
		{
			throw HTTP_EXCEPTION("foreign_api_version 2 not supported");
		}

		uint16_t highestVersion = 0;
		const Json::Value slateVersionsJSON = JsonUtil::GetRequiredField(okJSON, "supported_slate_versions");
		for (const Json::Value& slateVersion : slateVersionsJSON)
		{
			std::string versionStr(slateVersion.asCString());
			if (!versionStr.empty() && versionStr[0] == 'V')
			{
				uint16_t version = (uint16_t)std::stoul(versionStr.substr(1));
				if (version > highestVersion && version <= MAX_SLATE_VERSION)
				{
					highestVersion = version;
				}
			}
		}

		return highestVersion;
	}
	catch (const std::exception& e)
	{
		LOG_ERROR_F("Exception thrown: {}", e.what());
		throw HTTP_EXCEPTION("Error occurred during check_version.");
	}
}