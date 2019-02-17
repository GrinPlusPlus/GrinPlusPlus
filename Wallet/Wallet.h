#pragma once

#include "WalletCoin.h"
#include "Keychain/KeyChain.h"

#include <uuid.h>
#include <Common/Secure.h>
#include <Config/Config.h>
#include <Wallet/NodeClient.h>
#include <Wallet/SendMethod.h>
#include <Wallet/SelectionStrategy.h>
#include <Wallet/SlateContext.h>
#include <Wallet/WalletDB/WalletDB.h>
#include <Core/Models/TransactionOutput.h>

class Wallet
{
public:
	Wallet(const Config& config, const INodeClient& nodeClient, IWalletDB& walletDB, const std::string& username, KeyChainPath&& userPath);

	static Wallet* LoadWallet(const Config& config, const INodeClient& nodeClient, IWalletDB& walletDB, const std::string& username);

	std::vector<WalletCoin> GetAllAvailableCoins(const CBigInteger<32>& masterSeed) const;
	std::unique_ptr<WalletCoin> CreateBlindedOutput(const CBigInteger<32>& masterSeed, const uint64_t amount);

	bool SaveSlateContext(const uuids::uuid& slateId, const SlateContext& slateContext);
	bool LockCoins(std::vector<WalletCoin>& coins);

private:
	const Config& m_config;
	const INodeClient& m_nodeClient;
	IWalletDB& m_walletDB;
	KeyChain m_keyChain;
	std::string m_username; // Store Account (username and KeyChainPath), instead.
	KeyChainPath m_userPath;
};