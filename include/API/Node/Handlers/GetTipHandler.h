#pragma once

#include <Config/Config.h>
#include <BlockChain/BlockChainServer.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <Consensus/HardForks.h>
#include <optional>

class GetTipHandler : public RPCMethod
{
public:
	GetTipHandler(const IBlockChainServerPtr& pBlockChain)
		: m_pBlockChain(pBlockChain) { }
	~GetTipHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		auto pTip = m_pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED);

		Json::Value tipJson;
		tipJson["height"] = pTip->GetHeight();
		tipJson["last_block_pushed"] = pTip->GetHash().ToHex();
		tipJson["prev_block_to_last"] = pTip->GetPreviousHash().ToHex();
		tipJson["total_difficulty"] = pTip->GetTotalDifficulty();

		Json::Value result;
		result["Ok"] = tipJson;
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	IBlockChainServerPtr m_pBlockChain;
};