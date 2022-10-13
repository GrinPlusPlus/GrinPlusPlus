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
		Json::Value config_json = config.GetJSON();

		auto min_peers = JsonUtil::GetUInt32Opt(params_json, "min_peers");
		if (min_peers.has_value()) {
			if(min_peers.value() < config.GetMinSyncPeers()) {
				std::string error_message = "min_peers is expected to be >= than " + std::to_string(config.GetMinSyncPeers()) ;
				return request.BuildError("INVALID_PARAMS", error_message);
			}

			if (!config_json.isMember("P2P")) {
				config_json["P2P"] = Json::Value();
			}

			Json::Value& p2p_json = config_json["P2P"];
			p2p_json["MIN_PEERS"] = min_peers.value();

			config.SetMinPeers(min_peers.value());
		}

		auto max_peers = JsonUtil::GetUInt32Opt(params_json, "max_peers");
		if (max_peers.has_value()) {
			if (!config_json.isMember("P2P")) {
				config_json["P2P"] = Json::Value();
			}

			Json::Value& p2p_json = config_json["P2P"];
			p2p_json["MAX_PEERS"] = max_peers.value();

			config.SetMaxPeers(max_peers.value());
		}

		auto min_confirmations = JsonUtil::GetUInt32Opt(params_json, "min_confirmations");
		if (min_confirmations.has_value()) {
			if (!config_json.isMember("WALLET")) {
				config_json["WALLET"] = Json::Value();
			}

			Json::Value& wallet_json = config_json["WALLET"];
			wallet_json["MIN_CONFIRMATIONS"] = min_confirmations.value();

			config.SetMinConfirmations(min_confirmations.value());
		}

		auto reuse_address = JsonUtil::GetUInt32Opt(params_json, "reuse_address");
		if (reuse_address.has_value()) {
			if (!config_json.isMember("WALLET")) {
				config_json["WALLET"] = Json::Value();
			}

			Json::Value& wallet_json = config_json["WALLET"];
			wallet_json["REUSE_ADDRESS"] = reuse_address.value();

			config.ShouldReuseAddresses(reuse_address.value() == 0);
		}

		std::unordered_set<IPAddress> preferredPeers;
		std::vector<Json::Value>preferred_peers_json = JsonUtil::GetArray(params_json, "preferred_peers");
		std::transform(
			preferred_peers_json.cbegin(), preferred_peers_json.cend(),
			std::inserter(preferredPeers, preferredPeers.end()),
			[](const Json::Value& peer_json) {
				std::optional<std::string> peer = JsonUtil::ConvertToStringOpt(peer_json);
				return IPAddress::Parse(peer.value());
			}
		);
		
		if (!preferredPeers.empty()) {
			if (!config_json.isMember("P2P")) {
				config_json["P2P"] = Json::Value();
			}

			Json::Value& p2p_json = config_json["P2P"];
			Json::Value arr;
			for (const Json::Value& peer : preferred_peers_json)
			{
				arr.append(peer);
			}
			p2p_json["PREFERRED_PEERS"] = arr;

			config.UpdatePreferredPeers(preferredPeers);
		}

		std::unordered_set<IPAddress> allowedPeers;
		std::vector<Json::Value>allowed_peers_json = JsonUtil::GetArray(params_json, "allowed_peers");
		std::transform(
			allowed_peers_json.cbegin(), allowed_peers_json.cend(),
			std::inserter(allowedPeers, allowedPeers.end()),
			[](const Json::Value& peer_json) {
				std::optional<std::string> peer = JsonUtil::ConvertToStringOpt(peer_json);
				return IPAddress::Parse(peer.value());
			}
		);

		if (!allowedPeers.empty()) {
			if (!config_json.isMember("P2P")) {
				config_json["P2P"] = Json::Value();
			}

			Json::Value& p2p_json = config_json["P2P"];
			Json::Value arr;
			for (const Json::Value& peer : allowed_peers_json)
			{
				arr.append(peer);
			}
			p2p_json["ALLOWED_PEERS"] = arr;

			config.UpdateAllowedPeers(allowedPeers);
		}

		std::unordered_set<IPAddress> blockedPeers;
		std::vector<Json::Value>blocked_peers_json = JsonUtil::GetArray(params_json, "blocked_peers");
		std::transform(
			blocked_peers_json.cbegin(), blocked_peers_json.cend(),
			std::inserter(blockedPeers, blockedPeers.end()),
			[](const Json::Value& peer_json) {
				std::optional<std::string> peer = JsonUtil::ConvertToStringOpt(peer_json);
				return IPAddress::Parse(peer.value());
			}
		);

		if (!blockedPeers.empty()) {
			if (!config_json.isMember("P2P")) {
				config_json["P2P"] = Json::Value();
			}

			Json::Value& p2p_json = config_json["P2P"];
			Json::Value arr;
			for (const Json::Value& peer : blocked_peers_json)
			{
				arr.append(peer);
			}
			p2p_json["BLOCKED_PEERS"] = arr;

			config.UpdateBlockedPeers(blockedPeers);
		}
		
		fs::path config_path = config.GetDataDirectory() / "server_config.json";
		std::ofstream file(config_path, std::ios::out | std::ios::binary | std::ios::ate);
		if (file.is_open()) {
			file << config_json;
			file.close();

			Json::Value result_json;
			return request.BuildResult(result_json);
		} else {
			std::string error_message = "Failed to save config file at: " + config_path.u8string();
			LOG_ERROR(error_message);
			return request.BuildError("INTERNAL_ERROR", error_message);
		}		
	}

	bool ContainsSecrets() const noexcept final { return false; }
};