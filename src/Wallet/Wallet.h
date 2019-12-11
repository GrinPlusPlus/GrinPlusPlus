#pragma once

#include "Keychain/KeyChain.h"

#include <uuid.h>
#include <optional>
#include <Common/Secure.h>
#include <Config/Config.h>
#include <Wallet/NodeClient.h>
#include <Wallet/Enums/SelectionStrategy.h>
#include <Wallet/WalletSummary.h>
#include <Wallet/WalletTx.h>
#include <Wallet/WalletDB/WalletDB.h>
#include <Net/Tor/TorAddress.h>
#include <Core/Models/TransactionOutput.h>
#include <Core/Traits/Lockable.h>
#include <Crypto/SecretKey.h>
#include <Crypto/BulletproofType.h>
#include <string>

class Wallet
{
public:
	static Locked<Wallet> LoadWallet(
		const Config& config,
		INodeClientConstPtr pNodeClient,
		Locked<IWalletDB> walletDB,
		const std::string& username
	);

	const std::string& GetUsername() const { return m_username; }
	const KeyChainPath& GetUserPath() const { return m_userPath; }
	Locked<IWalletDB> GetDatabase() const { return m_walletDB; }

	void SetTorAddress(const TorAddress& address) { m_torAddressOpt = std::make_optional(address); }
	std::optional<TorAddress> GetTorAddress() const { return m_torAddressOpt; }
	void SetListenerPort(const uint16_t port) { m_listenerPort = port; }
	uint16_t GetListenerPort() const { return m_listenerPort; }

	WalletSummary GetWalletSummary(const SecureVector& masterSeed);

	std::unique_ptr<WalletTx> GetTxById(const SecureVector& masterSeed, const uint32_t walletTxId) const;
	std::unique_ptr<WalletTx> GetTxBySlateId(const SecureVector& masterSeed, const uuids::uuid& slateId) const;

	std::vector<OutputDataEntity> RefreshOutputs(const SecureVector& masterSeed, const bool fromGenesis);

	std::vector<OutputDataEntity> GetAllAvailableCoins(const SecureVector& masterSeed);
	OutputDataEntity CreateBlindedOutput(
		const SecureVector& masterSeed,
		const uint64_t amount,
		const KeyChainPath& keyChainPath,
		const uint32_t walletTxId,
		const EBulletproofType& bulletproofType,
		const std::optional<std::string>& messageOpt
	);

private:
	Wallet(
		const Config& config,
		INodeClientConstPtr pNodeClient,
		Locked<IWalletDB> walletDB,
		const std::string& username,
		KeyChainPath&& userPath
	);

	const Config& m_config;
	INodeClientConstPtr m_pNodeClient;
	Locked<IWalletDB> m_walletDB;
	std::string m_username; // Store Account (username and KeyChainPath), instead.
	KeyChainPath m_userPath;
	std::optional<TorAddress> m_torAddressOpt;
	uint16_t m_listenerPort;
};