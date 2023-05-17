#pragma once

#include <Consensus.h>
#include <BlockChain/BlockChain.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <optional>

class GetPeersHandler : public RPCMethod
{
public:
	GetPeersHandler(const IP2PServerPtr& pP2PServer)
		: m_pP2PServer(pP2PServer) { }
	~GetPeersHandler() = default;

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

		Json::Value json;

		if(values_json.size() == 0)
		{
			const std::vector<PeerConstPtr> peers = m_pP2PServer->GetAllPeers();
			for (PeerConstPtr peer : peers)
			{
				json.append(peer->ToJSON());
			}
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
			std::optional<PeerConstPtr> peerOpt = m_pP2PServer->GetPeer(peer);
			if (peerOpt.has_value())
			{
				json.append(peerOpt.value()->ToJSON());
			}
		}

		Json::Value result;
		result["Ok"] = json;

		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	IP2PServerPtr m_pP2PServer;
};