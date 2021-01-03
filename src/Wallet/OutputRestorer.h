#pragma once

#include <Wallet/NodeClient.h>
#include <Wallet/WalletDB/WalletDB.h>
#include <Wallet/WalletDB/Models/OutputDataEntity.h>
#include <Core/Config.h>
#include <Core/Models/DTOs/OutputDTO.h>

// Forward Declarations
class KeyChain;

class OutputRestorer
{
public:
	OutputRestorer(const Config& config, INodeClientConstPtr pNodeClient, const KeyChain& keyChain)
		: m_config(config), m_pNodeClient(pNodeClient), m_keyChain(keyChain) { }

	std::vector<OutputDataEntity> FindAndRewindOutputs(
		const std::shared_ptr<IWalletDB>& pBatch,
		const bool fromGenesis
	) const;

private:
	std::unique_ptr<OutputDataEntity> GetWalletOutput(
		const OutputDTO& output,
		const uint64_t currentBlockHeight
	) const;

	EOutputStatus DetermineStatus(
		const OutputDTO& output,
		const uint64_t currentBlockHeight
	) const;

	const Config& m_config;
	INodeClientConstPtr m_pNodeClient;
	const KeyChain& m_keyChain;
};