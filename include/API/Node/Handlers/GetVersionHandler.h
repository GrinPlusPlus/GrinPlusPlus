#pragma once

#include <Config/Config.h>
#include <BlockChain/BlockChainServer.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <GrinVersion.h>
#include <optional>

class GetVersionHandler : public RPCMethod
{
public:
	GetVersionHandler(const IBlockChainServerPtr& pBlockChain)
		: m_pBlockChain(pBlockChain) { }
	~GetVersionHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		Json::Value versionJson;
		versionJson["node_version"] = StringUtil::Format("Grin++ {}", GRINPP_VERSION);
		versionJson["block_header_version"] = m_pBlockChain->GetTipBlockHeader(EChainType::CANDIDATE)->GetVersion();

		Json::Value result;
		result["Ok"] = versionJson;
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	IBlockChainServerPtr m_pBlockChain;
};