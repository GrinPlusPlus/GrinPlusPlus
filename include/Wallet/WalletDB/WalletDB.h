#pragma once

#include <string>
#include <vector>

#include <uuid.h>
#include <Config/Config.h>
#include <Wallet/EncryptedSeed.h>
#include <Wallet/KeychainPath.h>
#include <Wallet/SlateContext.h>
#include <Wallet/OutputData.h>

#ifdef MW_WalletDB
#define WALLET_DB_API __declspec(dllexport)
#else
#define WALLET_DB_API __declspec(dllimport)
#endif

class IWalletDB
{
public:
	virtual bool CreateWallet(const std::string& username, const EncryptedSeed& encryptedSeed) = 0;

	virtual std::unique_ptr<EncryptedSeed> LoadWalletSeed(const std::string& username) const = 0;
	virtual KeyChainPath GetNextChildPath(const std::string& username, const KeyChainPath& parentPath) = 0;

	virtual bool SaveSlateContext(const std::string& username, const uuids::uuid& slateId, const SlateContext& slateContext) = 0;

	virtual bool AddOutputs(const std::string& username, const std::vector<OutputData>& outputs) = 0;
	virtual std::vector<OutputData> GetOutputs(const std::string& username) const = 0;
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