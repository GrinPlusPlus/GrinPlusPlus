#pragma once

#include "Keychain/KeyChain.h"

#include <uuid.h>
#include <Common/Secure.h>
#include <Config/Config.h>
#include <Wallet/NodeClient.h>
#include <Wallet/SelectionStrategy.h>
#include <Wallet/SlateContext.h>
#include <Wallet/WalletSummary.h>
#include <Wallet/WalletTx.h>
#include <Wallet/WalletDB/WalletDB.h>
#include <Core/Models/TransactionOutput.h>
#include <Crypto/SecretKey.h>

class Wallet
{
public:
	Wallet(const Config& config, const INodeClient& nodeClient, IWalletDB& walletDB, const std::string& username, KeyChainPath&& userPath);

	static Wallet* LoadWallet(const Config& config, const INodeClient& nodeClient, IWalletDB& walletDB, const std::string& username);

	WalletSummary GetWalletSummary(const SecretKey& masterSeed);

	std::vector<WalletTx> GetTransactions(const SecretKey& masterSeed);
	std::unique_ptr<WalletTx> GetTxById(const SecretKey& masterSeed, const uint32_t walletTxId);
	std::unique_ptr<WalletTx> GetTxBySlateId(const SecretKey& masterSeed, const uuids::uuid& slateId);

	std::vector<OutputData> RefreshOutputs(const SecretKey& masterSeed);
	bool AddRestoredOutputs(const SecretKey& masterSeed, const std::vector<OutputData>& outputs);

	uint32_t GetNextWalletTxId();
	bool AddWalletTxs(const SecretKey& masterSeed, const std::vector<WalletTx>& transactions);

	std::vector<OutputData> GetAllAvailableCoins(const SecretKey& masterSeed) const;
	OutputData CreateBlindedOutput(const SecretKey& masterSeed, const uint64_t amount);
	bool SaveOutputs(const SecretKey& masterSeed, const std::vector<OutputData>& outputsToSave);

	std::unique_ptr<SlateContext> GetSlateContext(const uuids::uuid& slateId, const SecretKey& masterSeed) const;
	bool SaveSlateContext(const uuids::uuid& slateId, const SecretKey& masterSeed, const SlateContext& slateContext);
	bool LockCoins(const SecretKey& masterSeed, std::vector<OutputData>& coins);

	bool CancelWalletTx(const SecretKey& masterSeed, WalletTx& walletTx);

private:
	const Config& m_config;
	const INodeClient& m_nodeClient;
	IWalletDB& m_walletDB;
	std::string m_username; // Store Account (username and KeyChainPath), instead.
	KeyChainPath m_userPath;
};