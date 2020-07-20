#pragma once

#include <Wallet/Keychain/KeyChain.h>

#include <uuid.h>
#include <optional>
#include <Common/Secure.h>
#include <Config/Config.h>
#include <Wallet/NodeClient.h>
#include <Wallet/Enums/SelectionStrategy.h>
#include <Wallet/Models/DTOs/WalletSummaryDTO.h>
#include <Wallet/Models/DTOs/WalletBalanceDTO.h>
#include <Wallet/Models/Slatepack/SlatepackAddress.h>
#include <Wallet/WalletTx.h>
#include <Wallet/WalletDB/WalletDB.h>
#include <Net/Tor/TorAddress.h>
#include <Core/Models/TransactionOutput.h>
#include <Core/Traits/Lockable.h>
#include <Crypto/SecretKey.h>
#include <Crypto/BulletproofType.h>
#include <API/Wallet/Foreign/Models/BuildCoinbaseResponse.h>
#include <string>

class Wallet
{
public:
	using Ptr = std::shared_ptr<Wallet>;
	using CPtr = std::shared_ptr<const Wallet>;

	static Locked<Wallet> LoadWallet(
		const Config& config,
		const SecureVector& masterSeed,
		INodeClientConstPtr pNodeClient,
		Locked<IWalletDB> walletDB,
		const std::string& username
	);

	const std::string& GetUsername() const { return m_username; }
	const KeyChainPath& GetUserPath() const { return m_userPath; }
	Locked<IWalletDB> GetDatabase() const { return m_walletDB; }
	const SlatepackAddress& GetSlatepackAddress() const noexcept { return m_address; }

	void SetTorAddress(const TorAddress& address) { m_torAddressOpt = std::make_optional(address); }
	std::optional<TorAddress> GetTorAddress() const { return m_torAddressOpt; }
	void SetListenerPort(const uint16_t port) { m_listenerPort = port; }
	uint16_t GetListenerPort() const { return m_listenerPort; }

	WalletSummaryDTO GetWalletSummary(const SecureVector& masterSeed);
	WalletBalanceDTO GetBalance(const SecureVector& masterSeed);

	std::unique_ptr<WalletTx> GetTxById(const SecureVector& masterSeed, const uint32_t walletTxId) const;
	std::unique_ptr<WalletTx> GetTxBySlateId(const SecureVector& masterSeed, const uuids::uuid& slateId) const;

	std::vector<OutputDataEntity> RefreshOutputs(const SecureVector& masterSeed, const bool fromGenesis);

	std::vector<OutputDataEntity> GetAllAvailableCoins(const SecureVector& masterSeed);
	OutputDataEntity CreateBlindedOutput(
		const SecureVector& masterSeed,
		const uint64_t amount,
		const KeyChainPath& keyChainPath,
		const uint32_t walletTxId,
		const EBulletproofType& bulletproofType
	);

	BuildCoinbaseResponse CreateCoinbase(
		const SecureVector& masterSeed,
		const uint64_t fees,
		const std::optional<KeyChainPath>& keyChainPathOpt
	);

private:
	Wallet(
		const Config& config,
		INodeClientConstPtr pNodeClient,
		Locked<IWalletDB> walletDB,
		const std::string& username,
		KeyChainPath&& userPath,
		const SlatepackAddress& address
	);

	const Config& m_config;
	INodeClientConstPtr m_pNodeClient;
	Locked<IWalletDB> m_walletDB;
	std::string m_username; // Store Account (username and KeyChainPath), instead.
	KeyChainPath m_userPath;
	SlatepackAddress m_address;
	std::optional<TorAddress> m_torAddressOpt;
	uint16_t m_listenerPort;
};