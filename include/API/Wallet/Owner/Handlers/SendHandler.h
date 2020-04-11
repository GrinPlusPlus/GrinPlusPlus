#pragma once

#include <Config/Config.h>
#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Tor/TorConnection.h>
#include <Net/Tor/TorManager.h>
#include <Net/Tor/TorAddressParser.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <Common/Util/FileUtil.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <optional>

class SendHandler : public RPCMethod
{
public:
	SendHandler(const Config& config, const IWalletManagerPtr& pWalletManager)
		: m_config(config), m_pWalletManager(pWalletManager) { }
	virtual ~SendHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
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

private:
	RPC::Response SendViaTOR(const RPC::Request& request, SendCriteria& criteria, const TorAddress& torAddress) const
	{
		uint16_t version = 0;

		try
		{
			TorConnectionPtr pTorConnection = TorManager::GetInstance(m_config.GetTorConfig()).Connect(torAddress);
			if (pTorConnection == nullptr)
			{
				return request.BuildError(RPC::Errors::RECEIVER_UNREACHABLE);
			}

			version = CheckVersion(*pTorConnection);
			if (version < MIN_SLATE_VERSION)
			{
				return request.BuildError(RPC::Errors::SLATE_VERSION_MISMATCH);
			}
		}
		catch (const HTTPException&)
		{
			return request.BuildError(RPC::Errors::RECEIVER_UNREACHABLE);
		}

		TorConnectionPtr pTorConnection = TorManager::GetInstance(m_config.GetTorConfig()).Connect(torAddress);
		if (pTorConnection == nullptr)
		{
			return request.BuildError(RPC::Errors::RECEIVER_UNREACHABLE);
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
			return request.BuildError(receiveTxResponse.GetError().value());
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

	RPC::Response SendViaFile(const RPC::Request& request, const SendCriteria& criteria, const std::string& file) const
	{
		// FUTURE: Check write permissions before creating slate

		Slate slate = m_pWalletManager->Send(criteria);

		Json::Value slateJSON = slate.ToJSON();
		try
		{
			FileUtil::WriteTextToFile(FileUtil::ToPath(file), JsonUtil::WriteCondensed(slateJSON));
			WALLET_INFO_F("Slate file saved to: {}", file);

			Json::Value result;
			result["status"] = "SENT";
			result["slate"] = slateJSON;
			return RPC::Response::BuildResult(request.GetId(), result);
		}
		catch (std::exception&)
		{
			WALLET_ERROR_F("Slate failed to save to: {}", file);

			Json::Value errorJson;
			errorJson["status"] = "WRITE_FAILED";
			errorJson["slate"] = slateJSON;
			return request.BuildError(RPC::ErrorCode::INTERNAL_ERROR, "Failed to write file.", errorJson);
		}
	}

	uint16_t CheckVersion(TorConnection& connection) const
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
		catch (const HTTPException& e)
		{
			LOG_ERROR_F("Exception thrown: {}", e.what());
			throw e;
		}
		catch (const std::exception& e)
		{
			LOG_ERROR_F("Exception thrown: {}", e.what());
			throw HTTP_EXCEPTION("Error occurred during check_version.");
		}
	}

	const Config& m_config;
	IWalletManagerPtr m_pWalletManager;
};