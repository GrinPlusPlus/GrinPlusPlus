#pragma once

#include "Node/NodeClient.h"

#include <Common/SecureString.h>
#include <Config/Config.h>
#include <Wallet/SendMethod.h>
#include <Wallet/SelectionStrategy.h>
#include <Core/TransactionOutput.h>

class Wallet
{
public:
	Wallet(const Config& config, INodeClient& nodeClient);

	static std::unique_ptr<Wallet> Initialize(const Config& config, INodeClient& nodeClient, const SecureString& password);
	static std::unique_ptr<Wallet> Load(const Config& config, INodeClient& nodeClient, const SecureString& password);

	bool Send(const uint64_t amount, const uint64_t fee, const std::string& message, const ESelectionStrategy& strategy, const ESendMethod& method, const std::string& destination); // TODO: Password?

	std::vector<TransactionOutput> GetAvailableOutputs(const ESelectionStrategy& strategy, const uint64_t amountWithFee);

private:
	const Config& m_config;
	INodeClient& m_nodeClient;
};