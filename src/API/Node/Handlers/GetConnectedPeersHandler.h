#pragma once

#include <Consensus.h>
#include <BlockChain/BlockChain.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <optional>

class GetConnectedPeersHandler : public RPCMethod
{
public:
	GetConnectedPeersHandler(const IP2PServerPtr& pP2PServer)
		: m_pP2PServer(pP2PServer) { }
	~GetConnectedPeersHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		const std::vector<ConnectedPeer> connectedPeers = m_pP2PServer->GetConnectedPeers();

		Json::Value json;
		for (const ConnectedPeer& connectedPeer : connectedPeers)
		{
			json.append(connectedPeer.ToJSON());
		}

		Json::Value result;
		result["Ok"] = json;

		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	IP2PServerPtr m_pP2PServer;
};