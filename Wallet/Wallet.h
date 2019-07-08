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
#include <Crypto/BulletproofType.h>

class Wallet
{
public:
	Wallet(const Config& config, const INodeClient& nodeClient, IWalletDB& walletDB, const std::string& username, KeyChainPath&& userPath);

	static Wallet* LoadWallet(const Config& config, const INodeClient& nodeClient, IWalletDB& walletDB, const std::string& username);

	inline const std::string& GetUsername() const { return m_username; }

	WalletSummary GetWalletSummary(const SecureVector& masterSeed);

	std::vector<WalletTx> GetTransactions(const SecureVector& masterSeed);
	std::unique_ptr<WalletTx> GetTxById(const SecureVector& masterSeed, const uint32_t walletTxId);
	std::unique_ptr<WalletTx> GetTxBySlateId(const SecureVector& masterSeed, const uuids::uuid& slateId);

	std::vector<OutputData> RefreshOutputs(const SecureVector& masterSeed, const bool fromGenesis);
	bool AddRestoredOutputs(const SecureVector& masterSeed, const std::vector<OutputData>& outputs);
	uint64_t GetRefreshHeight() const;
	bool SetRefreshHeight(const uint64_t blockHeight);
	uint64_t GetRestoreLeafIndex() const;
	bool SetRestoreLeafIndex(const uint64_t lastLeafIndex);

	uint32_t GetNextWalletTxId();
	bool AddWalletTxs(const SecureVector& masterSeed, const std::vector<WalletTx>& transactions);

	std::vector<OutputData> GetAllAvailableCoins(const SecureVector& masterSeed);
	OutputData CreateBlindedOutput(const SecureVector& masterSeed, const uint64_t amount, const uint32_t walletTxId, const EBulletproofType& bulletproofType);
	bool SaveOutputs(const SecureVector& masterSeed, const std::vector<OutputData>& outputsToSave);

	std::unique_ptr<SlateContext> GetSlateContext(const uuids::uuid& slateId, const SecureVector& masterSeed) const;
	bool SaveSlateContext(const uuids::uuid& slateId, const SecureVector& masterSeed, const SlateContext& slateContext);
	bool LockCoins(const SecureVector& masterSeed, std::vector<OutputData>& coins);

	bool CancelWalletTx(const SecureVector& masterSeed, WalletTx& walletTx);

private:
	const Config& m_config;
	const INodeClient& m_nodeClient;
	IWalletDB& m_walletDB;
	std::string m_username; // Store Account (username and KeyChainPath), instead.
	KeyChainPath m_userPath;
};