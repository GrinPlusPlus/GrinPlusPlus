#pragma once

#include <Common/Secure.h>
#include <optional>

#include <Core/Config.h>
#include <API/Wallet/Owner/Models/SendCriteria.h>
#include <API/Wallet/Owner/Models/ReceiveCriteria.h>
#include <API/Wallet/Owner/Models/FinalizeCriteria.h>
#include <API/Wallet/Owner/Models/ListTxsCriteria.h>
#include <API/Wallet/Owner/Models/RepostTxCriteria.h>
#include <API/Wallet/Owner/Models/EstimateFeeCriteria.h>
#include <API/Wallet/Foreign/Models/BuildCoinbaseCriteria.h>
#include <API/Wallet/Foreign/Models/BuildCoinbaseResponse.h>

#include <Wallet/Models/DTOs/WalletBalanceDTO.h>
#include <Wallet/Models/DTOs/WalletSummaryDTO.h>
#include <Wallet/Models/DTOs/WalletTxDTO.h>
#include <Wallet/Models/DTOs/FeeEstimateDTO.h>
#include <Wallet/Models/DTOs/SelectionStrategyDTO.h>
#include <Wallet/Models/Slatepack/Armor.h>
#include <Wallet/WalletDB/WalletDB.h>

#include <Net/Tor/TorAddress.h>
#include <Net/Tor/TorProcess.h>
#include <Crypto/SecretKey.h>

class Wallet
{
public:
	using Ptr = std::shared_ptr<Wallet>;
	using CPtr = std::shared_ptr<const Wallet>;

	Wallet(const Config::Ptr& pConfig, const Locked<IWalletDB>& walletDB, const SessionToken& token, const SecureVector& master_seed, const std::string& username, const KeyChainPath& user_path, const SlatepackAddress& address)
		: m_pConfig(pConfig), m_walletDB(walletDB), m_token(token), m_master_seed(master_seed), m_username(username), m_userPath(user_path), m_address(address) { }

	const std::string& GetUsername() const noexcept { return m_username; }
	const KeyChainPath& GetUserPath() const noexcept { return m_userPath; }
	const SlatepackAddress& GetSlatepackAddress() const noexcept { return m_address; }
	const SecureVector& GetMasterSeed() const noexcept { return m_master_seed; }

	void SetTorAddress(const TorAddress& address) noexcept { m_torAddressOpt = std::make_optional(address); }
	std::optional<TorAddress> GetTorAddress() const noexcept { return m_torAddressOpt; }
	void SetListenerPort(const uint16_t port) noexcept { m_listenerPort = port; }
	uint16_t GetListenerPort() const noexcept { return m_listenerPort; }

	SecureString GetSeedWords() const;
	WalletSummaryDTO GetWalletSummary() const;
	WalletBalanceDTO GetBalance() const;
	std::unique_ptr<WalletTx> GetTransactionById(const uint32_t txId) const;
	WalletTx GetTransactionBySlateId(const uuids::uuid& slateId, const EWalletTxType type) const;
	std::vector<WalletTxDTO> GetTransactions(const ListTxsCriteria& criteria) const;
	std::vector<WalletOutputDTO> GetOutputs(const bool includeSpent, const bool includeCanceled) const;
	Slate GetSlate(const uuids::uuid& slateId, const SlateStage& stage) const;
	SlateContextEntity GetSlateContext(const uuids::uuid& slateId) const;

	// void CheckForOutputs(const bool fromGenesis);

	std::optional<TorAddress> AddTorListener(const KeyChainPath& path, const TorProcess::Ptr& pTorProcess);

	FeeEstimateDTO EstimateFee(const EstimateFeeCriteria& criteria) const;

	// //
	// // Sends coins to the given destination using the specified method. Returns a valid slate if successful.
	// // Exceptions thrown:
	// // * SessionTokenException - If no matching session found, or if the token is invalid.
	// // * InsufficientFundsException - If there are not enough funds ready to spend after calculating and including the fee.
	// //
	// Slate Send(const SendCriteria& sendCriteria);

	// //
	// // Receives coins and builds a received slate to return to the sender.
	// //
	// Slate Receive(const ReceiveCriteria& receiveCriteria);

	// Slate Finalize(const Slate& slate, const std::optional<SlatepackMessage>& slatepackOpt);

	void CancelTx(const uint32_t walletTxId);

	BuildCoinbaseResponse BuildCoinbase(const BuildCoinbaseCriteria& criteria);
	SlatepackMessage DecryptSlatepack(const std::string& armoredSlatepack) const;

	// //
	// // Deletes the session information.
	// //
	// void Logout();

	// //
	// // Validates the password and then deletes the wallet.
	// //
	// void DeleteWallet(const SecureString& password);

	// void ChangePassword(const SecureString& currentPassword, const SecureString& newPassword);

	Locked<IWalletDB> GetDatabase() const { return m_walletDB; }

private:
	Config::Ptr m_pConfig;
	Locked<IWalletDB> m_walletDB;
	SessionToken m_token;
	SecureVector m_master_seed;
	std::string m_username; // Store Account (username and KeyChainPath), instead.
	KeyChainPath m_userPath;
	SlatepackAddress m_address;
	std::optional<TorAddress> m_torAddressOpt;
	uint16_t m_listenerPort;
};