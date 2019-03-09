#pragma once

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
	WalletRefresher(const INodeClient& nodeClient, IWalletDB& walletDB);

	std::vector<OutputData> RefreshOutputs(const std::string& username, const CBigInteger<32>& masterSeed, const uint64_t minimumConfirmations);

private:
	const INodeClient& m_nodeClient;
	IWalletDB& m_walletDB;
};