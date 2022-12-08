#pragma once

#include <P2P/P2PServer.h>
#include <BlockChain/BlockChain.h>
#include <P2P/Common.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <GrinVersion.h>

#include <optional>

class GetStatusHandler : public RPCMethod
{
public:
	GetStatusHandler(const IBlockChain::Ptr& pBlockChain, const IP2PServerPtr& pP2PServer)
		: m_pBlockChain(pBlockChain), m_pP2PServer(pP2PServer) { }
	virtual ~GetStatusHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		Json::Value result;

		auto pTip = m_pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED);
		if (pTip == nullptr)
		{
			return request.BuildError("ERROR", "Failed to find tip.");
		}

		Json::Value statusNode;
		statusNode["protocol_version"] = P2P::PROTOCOL_VERSION;
		statusNode["user_agent"] = P2P::USER_AGENT;

		SyncStatusConstPtr pSyncStatus = m_pP2PServer->GetSyncStatus();
		statusNode["sync_status"] = GetStatusString(pSyncStatus->GetStatus());

		Json::Value stateNode;
		stateNode["downloaded"] = pSyncStatus->GetDownloaded();
		stateNode["download_size"] = pSyncStatus->GetDownloadSize();
		stateNode["processing_status"] = pSyncStatus->GetProcessingStatus();
		statusNode["state"] = stateNode;

		Json::Value networkNode;
		networkNode["height"] = pSyncStatus->GetNetworkHeight();
		networkNode["total_difficulty"] = pSyncStatus->GetNetworkDifficulty();
		const std::pair<size_t, size_t> numConnections = m_pP2PServer->GetNumberOfConnectedPeers();
		networkNode["num_inbound"] = Json::UInt64(numConnections.first);
		networkNode["num_outbound"] = Json::UInt64(numConnections.second);
		statusNode["network"] = networkNode;

		Json::Value tipNode;
		tipNode["height"] = pTip->GetHeight();
		tipNode["hash"] = pTip->GetHash().ToHex();
		tipNode["previous_hash"] = pTip->GetPreviousHash().ToHex();
		tipNode["total_difficulty"] = pTip->GetTotalDifficulty();
		statusNode["chain"] = tipNode;

		const uint64_t headerHeight = m_pBlockChain->GetHeight(EChainType::CANDIDATE);
		statusNode["header_height"] = headerHeight;

		result["Ok"] = statusNode;
		return request.BuildResult(result);
	}
	
	bool ContainsSecrets() const noexcept final { return false; }

private:
	std::string GetStatusString(const ESyncStatus& status) const noexcept
	{
		switch (status)
		{
			case ESyncStatus::SYNCING_HEADERS:
			{
				return "header_sync";
			}
			case ESyncStatus::SYNCING_TXHASHSET:
			case ESyncStatus::TXHASHSET_SYNC_FAILED:
			{
				return "txhashset_download";
			}
			case ESyncStatus::PROCESSING_TXHASHSET:
			{
				return "processing_txhashset";
			}
			case ESyncStatus::SYNCING_BLOCKS:
			{
				return "body_sync";
			}
			case ESyncStatus::WAITING_FOR_PEERS:
			{
				return "not_connected";
			}
			case ESyncStatus::NOT_SYNCING:
			{
				return "no_sync";
			}
		}

		return "unknown";
	}

	IBlockChain::Ptr m_pBlockChain;
	IP2PServerPtr m_pP2PServer;
};
