#pragma once

#include <Config/Config.h>
#include <Wallet/OutputData.h>
#include <Wallet/WalletTx.h>
#include <Wallet/NodeClient.h>
#include <Wallet/WalletDB/WalletDB.h>
#include <Common/Secure.h>
#include <Crypto/SecretKey.h>
#include <stdint.h>
#include <string>

// Forward Declarations
class INodeClient;
class Wallet;

class WalletRefresher
{
public:
	WalletRefresher(const Config& config, INodeClientConstPtr pNodeClient);

	std::vector<OutputData> Refresh(const SecureVector& masterSeed, Locked<IWalletDB> walletDB, const bool fromGenesis);

private:
	void RefreshOutputs(const SecureVector& masterSeed, Writer<IWalletDB> pBatch, std::vector<OutputData>& walletOutputs);
	void RefreshTransactions(const SecureVector& masterSeed, Writer<IWalletDB> pBatch, const std::vector<OutputData>& walletOutputs, std::vector<WalletTx>& walletTransactions);
	std::optional<std::chrono::system_clock::time_point> GetBlockTime(const OutputData& output) const;

	std::unique_ptr<OutputData> FindOutput(const std::vector<OutputData>& walletOutputs, const Commitment& commitment) const;

	const Config& m_config;
	INodeClientConstPtr m_pNodeClient;
};