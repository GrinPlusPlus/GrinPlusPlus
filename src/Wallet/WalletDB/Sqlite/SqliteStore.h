#pragma once

#include <Wallet/WalletDB/WalletStore.h>
#include <libsqlite3/sqlite3.h>
#include <string>
#include <unordered_map>

class SqliteStore : public IWalletStore
{
public:
	static std::shared_ptr<SqliteStore> Open(const Config& config);
	virtual ~SqliteStore() = default;

	virtual std::vector<std::string> GetAccounts() const override final;
	virtual Locked<IWalletDB> OpenWallet(const std::string& username, const SecureVector& masterSeed) override final;
	virtual Locked<IWalletDB> CreateWallet(const std::string& username, const EncryptedSeed& encryptedSeed) override final;
	virtual EncryptedSeed LoadWalletSeed(const std::string& username) const override final;

private:
	SqliteStore(const std::string& walletDirectory) : m_walletDirectory(walletDirectory) { }

	sqlite3* CreateWalletDB(const std::string& username);
	std::string GetDBFile(const std::string& username) const;

	const std::string& m_walletDirectory;
	std::unordered_map<std::string, Locked<IWalletDB>> m_userDBs;
};