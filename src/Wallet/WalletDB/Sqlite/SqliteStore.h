#pragma once

#include "SqliteDB.h"

#include <Common/Secure.h>
#include <Wallet/WalletDB/WalletStore.h>
#include <filesystem.h>
#include <string>
#include <unordered_map>

class SqliteStore : public IWalletStore
{
public:
	static std::shared_ptr<SqliteStore> Open(const Config& config);
	virtual ~SqliteStore() = default;

	std::vector<GrinStr> GetAccounts() const final;
	Locked<IWalletDB> OpenWallet(const std::string& username, const SecureVector& masterSeed) final;
	void CloseWallet(const std::string& username) final;
	Locked<IWalletDB> CreateWallet(const std::string& username, const EncryptedSeed& encryptedSeed) final;
	void DeleteWallet(const std::string& username) final;
	void ChangePassword(const std::string& username, const EncryptedSeed& encryptedSeed) final;
	EncryptedSeed LoadWalletSeed(const std::string& username) const final;

private:
	SqliteStore(const fs::path& walletDirectory) : m_walletDirectory(walletDirectory) { }

	SqliteDB::Ptr CreateWalletDB(const std::string& username);
	fs::path GetDBFile(const std::string& username) const;

	const fs::path& m_walletDirectory;
	std::unordered_map<std::string, Locked<IWalletDB>> m_userDBs;
};