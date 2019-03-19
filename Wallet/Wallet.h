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

class Wallet
{
public:
	Wallet(const Config& config, const INodeClient& nodeClient, IWalletDB& walletDB, const std::string& username, KeyChainPath&& userPath);

	static Wallet* LoadWallet(const Config& config, const INodeClient& nodeClient, IWalletDB& walletDB, const std::string& username);

	WalletSummary GetWalletSummary(const CBigInteger<32>& masterSeed);

	std::vector<WalletTx> GetTransactions(const CBigInteger<32>& masterSeed);
	std::unique_ptr<WalletTx> GetTxById(const CBigInteger<32>& masterSeed, const uint32_t walletTxId);
	std::unique_ptr<WalletTx> GetTxBySlateId(const CBigInteger<32>& masterSeed, const uuids::uuid& slateId);

	std::vector<OutputData> RefreshOutputs(const CBigInteger<32>& masterSeed);
	bool AddRestoredOutputs(const CBigInteger<32>& masterSeed, const std::vector<OutputData>& outputs);

	uint32_t GetNextWalletTxId();
	bool AddWalletTxs(const CBigInteger<32>& masterSeed, const std::vector<WalletTx>& transactions);

	std::vector<OutputData> GetAllAvailableCoins(const CBigInteger<32>& masterSeed) const;
	std::unique_ptr<OutputData> CreateBlindedOutput(const CBigInteger<32>& masterSeed, const uint64_t amount);

	std::unique_ptr<SlateContext> GetSlateContext(const uuids::uuid& slateId, const CBigInteger<32>& masterSeed) const;
	bool SaveSlateContext(const uuids::uuid& slateId, const CBigInteger<32>& masterSeed, const SlateContext& slateContext);
	bool LockCoins(const CBigInteger<32>& masterSeed, std::vector<OutputData>& coins);

	bool CancelWalletTx(const CBigInteger<32>& masterSeed, WalletTx& walletTx);

private:
	const Config& m_config;
	const INodeClient& m_nodeClient;
	IWalletDB& m_walletDB;
	std::string m_username; // Store Account (username and KeyChainPath), instead.
	KeyChainPath m_userPath;
};