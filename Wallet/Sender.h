#pragma once

#include "Wallet.h"
#include "Node/NodeClient.h"

#include <Wallet/Slate.h>
#include <BlockChainServer.h>

class Sender
{
public:
	Sender(const INodeClient& nodeClient);

	// Creates a slate for sending grins from the provided wallet.
	std::unique_ptr<Slate> BuildSendSlate(Wallet& wallet, const uint64_t amount, const uint64_t fee, const std::string& message, const ESelectionStrategy& strategy) const; // TODO: Password?

private:
	const INodeClient& m_nodeClient;
};