#pragma once

#include "../../civetweb/include/civetweb.h"

#include <Wallet/SessionToken.h>

// Forward Declarations
class IWalletManager;
class INodeClient;

class OwnerGetAPI
{
public:
	static int HandleGET(mg_connection* pConnection, const std::string& action, IWalletManager& walletManager, INodeClient& nodeClient);

private:
	static int GetNodeHeight(mg_connection* pConnection, INodeClient& nodeClient);
	static int RetrieveSummaryInfo(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token);
	static int RetrieveTransactions(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token);
	static int RetrieveOutputs(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token);
	static int EstimateFee(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token);
};