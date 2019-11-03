#pragma once

#include "Wallet.h"

#include <Wallet/NodeClient.h>
#include <Config/Config.h>
#include <Core/Models/Display/OutputDisplayInfo.h>

class OutputRestorer
{
public:
	OutputRestorer(const Config& config, INodeClientConstPtr pNodeClient, const KeyChain& keyChain);

	std::vector<OutputData> FindAndRewindOutputs(const SecureVector& masterSeed, Wallet& wallet, const bool fromGenesis) const;

private:
	std::unique_ptr<OutputData> GetWalletOutput(const SecureVector& masterSeed, const OutputDisplayInfo& outputDisplayInfo, const uint64_t currentBlockHeight) const;
	EOutputStatus DetermineStatus(const OutputDisplayInfo& outputDisplayInfo, const uint64_t currentBlockHeight) const;

	const Config& m_config;
	INodeClientConstPtr m_pNodeClient;
	const KeyChain& m_keyChain;
};