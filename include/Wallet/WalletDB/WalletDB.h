#pragma once

#include <string>
#include <vector>

#include <uuid.h>
#include <Config/Config.h>
#include <Crypto/SecretKey.h>
#include <Wallet/EncryptedSeed.h>
#include <Wallet/KeyChainPath.h>
#include <Wallet/SlateContext.h>
#include <Wallet/OutputData.h>
#include <Wallet/WalletTx.h>

#ifdef MW_WalletDB
#define WALLET_DB_API EXPORT
#else
#define WALLET_DB_API IMPORT
#endif

class IWalletDB
{
public:
	virtual std::vector<std::string> GetAccounts() const = 0;

	virtual bool OpenWallet(const std::string& username) = 0;
	virtual bool CreateWallet(const std::string& username, const EncryptedSeed& encryptedSeed) = 0;

	virtual std::unique_ptr<EncryptedSeed> LoadWalletSeed(const std::string& username) const = 0;
	virtual KeyChainPath GetNextChildPath(const std::string& username, const KeyChainPath& parentPath) = 0;

	virtual std::unique_ptr<SlateContext> LoadSlateContext(const std::string& username, const SecureVector& masterSeed, const uuids::uuid& slateId) const = 0;
	virtual bool SaveSlateContext(const std::string& username, const SecureVector& masterSeed, const uuids::uuid& slateId, const SlateContext& slateContext) = 0;

	virtual bool AddOutputs(const std::string& username, const SecureVector& masterSeed, const std::vector<OutputData>& outputs) = 0;
	virtual std::vector<OutputData> GetOutputs(const std::string& username, const SecureVector& masterSeed) const = 0;

	virtual bool AddTransaction(const std::string& username, const SecureVector& masterSeed, const WalletTx& walletTx) = 0;
	virtual std::vector<WalletTx> GetTransactions(const std::string& username, const SecureVector& masterSeed) const = 0;

	virtual uint32_t GetNextTransactionId(const std::string& username) = 0;
	virtual uint64_t GetRefreshBlockHeight(const std::string& username) const = 0;
	virtual bool UpdateRefreshBlockHeight(const std::string& username, const uint64_t refreshBlockHeight) = 0;
	virtual uint64_t GetRestoreLeafIndex(const std::string& username) const = 0;
	virtual bool UpdateRestoreLeafIndex(const std::string& username, const uint64_t lastLeafIndex) = 0;
};

namespace WalletDBAPI
{
	//
	// Opens the wallet database and returns an instance of IWalletDB.
	//
	WALLET_DB_API IWalletDB* OpenWalletDB(const Config& config);

	//
	// Closes the wallet database and cleans up the memory of IWalletDB.
	//
	WALLET_DB_API void CloseWalletDB(IWalletDB* pWalletDB);
}