#pragma once

#include <Consensus.h>
#include <BlockChain/BlockChain.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <optional>

class GetStatusHandler : public RPCMethod
{
public:
	GetStatusHandler(const IBlockChain::Ptr& pBlockChain, const IP2PServerPtr& pP2PServer)
		: m_pBlockChain(pBlockChain), m_pP2PServer(pP2PServer) { }
	~GetStatusHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		Json::Value statusNode;
		statusNode["protocol_version"] = P2P::PROTOCOL_VERSION;
		statusNode["user_agent"] = P2P::USER_AGENT;

		auto pTip = m_pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED);

		if (pTip == nullptr)
		{
			return request.BuildError("INVALID_CHAIN_STATUS", "Failed to find tip.");
		}

		Json::Value tipNode;
		tipNode["height"] = pTip->GetHeight();
		tipNode["last_block_pushed"] = pTip->GetHash().ToHex();
		tipNode["prev_block_to_last"] = pTip->GetPreviousHash().ToHex();
		tipNode["total_difficulty"] = pTip->GetTotalDifficulty();
		statusNode["tip"] = tipNode;	

		SyncStatusConstPtr pSyncStatus = m_pP2PServer->GetSyncStatus();
		statusNode["sync_status"] = GetStatusString(*pSyncStatus);
		
		Json::Value syncInfo;
		switch (pSyncStatus->GetStatus())
			{
			case ESyncStatus::SYNCING_HEADERS:
			{
				syncInfo["highest_height"] = pSyncStatus->GetNetworkHeight();
				syncInfo["current_height"] = m_pBlockChain->GetHeight(EChainType::CANDIDATE);
			}
			case ESyncStatus::TXHASHSET_SYNC_FAILED:
			{
				syncInfo["downloaded_size"] = pSyncStatus->GetDownloaded();
				syncInfo["total_size"] = pSyncStatus->GetDownloadSize();
				syncInfo["processing_status"] = pSyncStatus->GetProcessingStatus();
			}
			case ESyncStatus::PROCESSING_TXHASHSET:
			{
				syncInfo["downloaded_size"] = pSyncStatus->GetDownloaded();
				syncInfo["total_size"] = pSyncStatus->GetDownloadSize();
			}
		}
		statusNode["sync_info"] = syncInfo;

		Json::Value result;
		result["Ok"] = statusNode;
		return request.BuildResult(result);
	}

	static std::string GetStatusString(const SyncStatus& syncStatus)
	{
		const ESyncStatus status = syncStatus.GetStatus();

		switch (status)
		{
			case ESyncStatus::SYNCING_HEADERS:
			{
				return "header_sync";
			}
			case ESyncStatus::SYNCING_TXHASHSET:
			{
				return "syncing";
			}
			case ESyncStatus::TXHASHSET_SYNC_FAILED:
			{
				return "txhashset_download";
			}
			case ESyncStatus::PROCESSING_TXHASHSET:
			{
				return "txhashset_processing";
			}
			case ESyncStatus::SYNCING_BLOCKS:
			{
				return "body_sync";
			}
			case ESyncStatus::WAITING_FOR_PEERS:
			{
				return "awating_peers";
			}
			case ESyncStatus::NOT_SYNCING:
			{
				return "no_sync";
			}
		}

		return "unknown";
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	IBlockChain::Ptr m_pBlockChain;
	IP2PServerPtr m_pP2PServer;
};



