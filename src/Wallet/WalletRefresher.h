#pragma once

#include <Core/Config.h>
#include <Wallet/WalletTx.h>
#include <Wallet/NodeClient.h>
#include <Wallet/WalletDB/WalletDB.h>
#include <Wallet/WalletDB/Models/OutputDataEntity.h>
#include <Common/Secure.h>
#include <Crypto/SecretKey.h>
#include <cstdint>
#include <string>

// Forward Declarations
class INodeClient;
class Wallet;

class WalletRefresher
{
public:
	WalletRefresher(const Config& config, const INodeClientConstPtr& pNodeClient)
		: m_config(config), m_pNodeClient(pNodeClient) { }

	std::vector<OutputDataEntity> Refresh(
		const SecureVector& masterSeed,
		Locked<IWalletDB> walletDB,
		const bool fromGenesis
	);

private:
	void RefreshOutputs(
		const SecureVector& masterSeed,
		Writer<IWalletDB> pBatch,
		std::vector<OutputDataEntity>& walletOutputs
	);

	void RefreshTransactions(
		const SecureVector& masterSeed,
		Writer<IWalletDB> pBatch,
		const std::vector<OutputDataEntity>& walletOutputs,
		std::vector<WalletTx>& walletTransactions
	);

	std::optional<std::chrono::system_clock::time_point> GetBlockTime(const OutputDataEntity& output) const;

	std::unique_ptr<OutputDataEntity> FindOutput(
		const std::vector<OutputDataEntity>& walletOutputs,
		const Commitment& commitment
	) const;

	const Config& m_config;
	INodeClientConstPtr m_pNodeClient;
};