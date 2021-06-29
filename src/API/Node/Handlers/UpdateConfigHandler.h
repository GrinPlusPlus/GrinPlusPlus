#pragma once

#include <Core/Config.h>
#include <Core/Global.h>
#include <Core/Util/JsonUtil.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <optional>
#include <fstream>

class UpdateConfigHandler : public RPCMethod
{
public:
	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value()) {
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		const Json::Value params_json = request.GetParams().value();
		if (!params_json.isObject()) {
			return request.BuildError("INVALID_PARAMS", "Expected object parameter");
		}

		Config& config = Global::GetConfig();
		Json::Value json = config.GetJSON();

		auto min_peers = JsonUtil::GetUInt32Opt(params_json, "min_peers");
		if (min_peers.has_value()) {
			if (!json.isMember("P2P")) {
				json["P2P"] = Json::Value();
			}

			Json::Value& p2p_json = json["P2P"];
			p2p_json["MIN_PEERS"] = min_peers.value();

			config.SetMinPeers(min_peers.value());
		}

		auto max_peers = JsonUtil::GetUInt32Opt(params_json, "max_peers");
		if (max_peers.has_value()) {
			if (!json.isMember("P2P")) {
				json["P2P"] = Json::Value();
			}

			Json::Value& p2p_json = json["P2P"];
			p2p_json["MAX_PEERS"] = max_peers.value();

			config.SetMaxPeers(max_peers.value());
		}

		auto min_confirmations = JsonUtil::GetUInt32Opt(params_json, "min_confirmations");
		if (min_confirmations.has_value()) {
			if (!json.isMember("WALLET")) {
				json["WALLET"] = Json::Value();
			}

			Json::Value& wallet_json = json["WALLET"];
			wallet_json["MIN_CONFIRMATIONS"] = min_confirmations.value();

			config.SetMinConfirmations(min_confirmations.value());
		}

		auto reuse_address = JsonUtil::GetUInt32Opt(params_json, "reuse_address");
		if (reuse_address.has_value()) {
			if (!json.isMember("WALLET")) {
				json["WALLET"] = Json::Value();
			}

			Json::Value& wallet_json = json["WALLET"];
			wallet_json["REUSE_ADDRESS"] = reuse_address.value();

			config.ShouldReuseAddresses(reuse_address.value() == 0 ? false: true);
		}

		fs::path config_path = config.GetDataDirectory() / "server_config.json";
		std::ofstream file(config_path, std::ios::out | std::ios::binary | std::ios::ate);
		if (!file.is_open()) {
			LOG_ERROR_F("Failed to save config file at: {}", config_path);
			throw FILE_EXCEPTION_F("Failed to save config file at: {}", config_path);
		}

		file << json;
		file.close();

		Json::Value result_json;
		return request.BuildResult(result_json);
	}

	bool ContainsSecrets() const noexcept final { return false; }
};