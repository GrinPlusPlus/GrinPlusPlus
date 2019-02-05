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
#include <Core/TransactionOutput.h>

class Wallet
{
public:
	Wallet(const Config& config, INodeClient& nodeClient, IWalletDB& walletDB, KeyChain&& keyChain, const std::string& username, KeyChainPath&& userPath);

	static Wallet* LoadWallet(const Config& config, INodeClient& nodeClient, IWalletDB& walletDB, const std::string& username, const EncryptedSeed& encryptedSeed);

	bool Send(const SecureString& password, const uint64_t amount, const uint64_t fee, const std::string& message, const ESelectionStrategy& strategy, const ESendMethod& method, const std::string& destination);

	std::vector<WalletCoin> GetAllAvailableCoins(const SecureString& password) const;
	std::unique_ptr<WalletCoin> CreateBlindedOutput(const uint64_t amount, const SecureString& password);

	bool SaveSlateContext(const uuids::uuid& slateId, const SlateContext& slateContext);
	bool LockCoins(const std::vector<WalletCoin>& coins);

private:
	const Config& m_config;
	INodeClient& m_nodeClient;
	IWalletDB& m_walletDB;
	KeyChain m_keyChain;
	std::string m_username; // Store Account (username and KeyChainPath), instead.
	KeyChainPath m_userPath;
};