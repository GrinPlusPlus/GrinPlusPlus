#pragma once

#include "Wallet.h"

#include <Wallet/NodeClient.h>
#include <Config/Config.h>
#include <PMMR/OutputRange.h>

// Forward Declarations
class RewoundProof;

class WalletRestorer
{
public:
	WalletRestorer(const Config& config, const INodeClient& nodeClient);

	bool Restore(const CBigInteger<32>& masterSeed, Wallet& wallet) const;

private:
	std::unique_ptr<OutputData> GetWalletOutput(const CBigInteger<32>& masterSeed, const OutputDisplayInfo& outputDisplayInfo, const uint64_t currentBlockHeight) const;
	EOutputStatus DetermineStatus(const OutputDisplayInfo& outputDisplayInfo, const uint64_t currentBlockHeight) const;

	const Config& m_config;
	const INodeClient& m_nodeClient;
};