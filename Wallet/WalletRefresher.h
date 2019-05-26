#pragma once

#include <Config/Config.h>
#include <Wallet/OutputData.h>
#include <Wallet/WalletTx.h>
#include <Common/Secure.h>
#include <Crypto/SecretKey.h>
#include <stdint.h>
#include <string>

// Forward Declarations
class INodeClient;
class IWalletDB;
class Wallet;

class WalletRefresher
{
public:
	WalletRefresher(const Config& config, const INodeClient& nodeClient, IWalletDB& walletDB);

	std::vector<OutputData> Refresh(const SecureVector& masterSeed, Wallet& wallet, const bool fromGenesis);

private:
	void RefreshOutputs(const SecureVector& masterSeed, Wallet& wallet, std::vector<OutputData>& walletOutputs);
	void RefreshTransactions(const std::string& username, const SecureVector& masterSeed, const std::vector<OutputData>& walletOutputs, std::vector<WalletTx>& walletTransactions);

	std::unique_ptr<OutputData> FindOutput(const std::vector<OutputData>& walletOutputs, const Commitment& commitment) const;

	const Config& m_config;
	const INodeClient& m_nodeClient;
	IWalletDB& m_walletDB;
};