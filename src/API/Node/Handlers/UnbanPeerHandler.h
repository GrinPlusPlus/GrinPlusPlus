#pragma once

#include <Consensus.h>
#include <BlockChain/BlockChain.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <optional>

class UnbanPeerHandler : public RPCMethod
{
public:
	UnbanPeerHandler(const IP2PServerPtr& pP2PServer)
		: m_pP2PServer(pP2PServer) { }
	~UnbanPeerHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value()) {
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		const Json::Value params = request.GetParams().value();
		if (!params.isArray()) {
			return request.BuildError("INVALID_PARAMS", "Expected array");
		}

		std::vector<Json::Value> values_json;
		for (auto iter = params.begin(); iter != params.end(); iter++)
		{
			values_json.push_back(*iter);
		}

		if (values_json.size() == 0)
		{
			return request.BuildError("INVALID_PARAMS", "Empty array");
		}
		else
		{
			std::unordered_set<IPAddress> peersWanted;

			std::transform(
				values_json.cbegin(), values_json.cend(),
				std::inserter(peersWanted, peersWanted.end()),
				[](const Json::Value& peer_json) {
					std::optional<std::string> peer = JsonUtil::ConvertToStringOpt(peer_json);
					return IPAddress::Parse(peer.value());
				}
			);
			auto peer = *(peersWanted.begin());
			m_pP2PServer->UnbanPeer(peer);
		}

		Json::Value result;
		result["Ok"] = "";

		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	IP2PServerPtr m_pP2PServer;
};