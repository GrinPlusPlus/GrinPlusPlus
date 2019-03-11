#pragma once

#include <Config/Config.h>
#include <Wallet/OutputData.h>
#include <Crypto/BigInteger.h>
#include <stdint.h>
#include <string>

// Forward Declarations
class INodeClient;
class IWalletDB;

class WalletRefresher
{
public:
	WalletRefresher(const Config& config, const INodeClient& nodeClient, IWalletDB& walletDB);

	std::vector<OutputData> RefreshOutputs(const std::string& username, const CBigInteger<32>& masterSeed);

private:
	const Config& m_config;
	const INodeClient& m_nodeClient;
	IWalletDB& m_walletDB;
};