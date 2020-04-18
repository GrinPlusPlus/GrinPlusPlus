#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Common/ImportExport.h>
#include <Config/Config.h>
#include <Wallet/NodeClient.h>
#include <Wallet/SessionToken.h>
#include <Wallet/Enums/SelectionStrategy.h>
#include <Wallet/Models/Slate/Slate.h>
#include <Wallet/Models/DTOs/WalletSummaryDTO.h>
#include <Wallet/WalletTx.h>
#include <API/Wallet/Owner/Models/GetSeedPhraseCriteria.h>
#include <API/Wallet/Owner/Models/CreateWalletCriteria.h>
#include <API/Wallet/Owner/Models/CreateWalletResponse.h>
#include <API/Wallet/Owner/Models/RestoreWalletCriteria.h>
#include <API/Wallet/Owner/Models/LoginCriteria.h>
#include <API/Wallet/Owner/Models/LoginResponse.h>
#include <API/Wallet/Owner/Models/SendCriteria.h>
#include <API/Wallet/Owner/Models/ReceiveCriteria.h>
#include <API/Wallet/Owner/Models/FinalizeCriteria.h>
#include <API/Wallet/Owner/Models/ListTxsCriteria.h>
#include <API/Wallet/Foreign/Models/BuildCoinbaseCriteria.h>
#include <API/Wallet/Foreign/Models/BuildCoinbaseResponse.h>
#include <Wallet/Models/DTOs/WalletTxDTO.h>
#include <Wallet/Models/DTOs/FeeEstimateDTO.h>
#include <Wallet/Models/DTOs/SelectionStrategyDTO.h>
#include <Net/Tor/TorAddress.h>
#include <Net/Tor/TorProcess.h>
#include <Crypto/SecretKey.h>
#include <Common/Secure.h>

#ifdef MW_WALLET
#define WALLET_API EXPORT
#else
#define WALLET_API IMPORT
#endif

class IWalletManager
{
public:
	virtual ~IWalletManager() = default;

	//
	// Creates a new wallet with the username and password given, and returns the space-delimited wallet words (BIP39 mnemonics).
	// If a wallet for the user already exists, an empty string will be returned.
	//
	virtual CreateWalletResponse InitializeNewWallet(
		const CreateWalletCriteria& criteria,
		const TorProcess::Ptr& pTorProcess
	) = 0;

	//
	// Creates a wallet from existing wallet words (BIP39 mnemonics).
	// Restoring outputs must be done separately.
	//
	virtual LoginResponse RestoreFromSeed(
		const RestoreWalletCriteria& criteria,
		const TorProcess::Ptr& pTorProcess
	) = 0;

	//
	// Returns the logged in user's wallet words.
	//
	virtual SecureString GetSeedWords(const SessionToken& token) = 0;
	virtual SecureString GetSeedWords(const GetSeedPhraseCriteria& criteria) = 0;

	virtual void CheckForOutputs(
		const SessionToken& token,
		const bool fromGenesis
	) = 0;

	virtual std::vector<std::string> GetAllAccounts() const = 0;

	virtual std::optional<TorAddress> GetTorAddress(const SessionToken& token) const = 0;

	virtual std::optional<TorAddress> AddTorListener(
		const SessionToken& token,
		const KeyChainPath& path,
		const TorProcess::Ptr& pTorProcess
	) = 0;

	virtual uint16_t GetListenerPort(const SessionToken& token) const = 0;

	//
	// Authenticates the user, and if successful, returns a session token that can be used in lieu of credentials for future calls.
	//
	virtual LoginResponse Login(
		const LoginCriteria& criteria,
		const TorProcess::Ptr& pTorProcess
	) = 0;

	//
	// Deletes the session information.
	//
	virtual void Logout(const SessionToken& token) = 0;

	//
	// Validates the password and then deletes the wallet.
	//
	virtual void DeleteWallet(
		const std::string& username,
		const SecureString& password
	) = 0;

	virtual void ChangePassword(
		const std::string& username,
		const SecureString& currentPassword,
		const SecureString& newPassword
	) = 0;

	virtual WalletSummaryDTO GetWalletSummary(const SessionToken& token) = 0;

	virtual std::vector<WalletTxDTO> GetTransactions(const ListTxsCriteria& criteria) = 0;

	virtual std::vector<WalletOutputDTO> GetOutputs(
		const SessionToken& token, 
		const bool includeSpent, 
		const bool includeCanceled
	) = 0;

	virtual FeeEstimateDTO EstimateFee(
		const SessionToken& token,
		const uint64_t amountToSend,
		const uint64_t feeBase,
		const SelectionStrategyDTO& strategy,
		const uint8_t numChangeOutputs
	) = 0;

	//
	// Sends coins to the given destination using the specified method. Returns a valid slate if successful.
	// Exceptions thrown:
	// * SessionTokenException - If no matching session found, or if the token is invalid.
	// * InsufficientFundsException - If there are not enough funds ready to spend after calculating and including the fee.
	//
	virtual Slate Send(const SendCriteria& sendCriteria) = 0;

	//
	// Receives coins and builds a received slate to return to the sender.
	//
	virtual Slate Receive(const ReceiveCriteria& receiveCriteria) = 0;

	virtual Slate Finalize(
		const FinalizeCriteria& finalizeCriteria,
		const TorProcess::Ptr & pTorProcess
	) = 0;

	virtual bool PostTransaction(
		const Transaction& transaction,
		const PostMethodDTO& postMethod,
		const TorProcess::Ptr& pTorProcess
	) = 0;

	virtual bool RepostByTxId(
		const SessionToken& token,
		const uint32_t walletTxId
	) = 0;

	virtual void CancelByTxId(
		const SessionToken& token,
		const uint32_t walletTxId
	) = 0;

	virtual BuildCoinbaseResponse BuildCoinbase(
		const BuildCoinbaseCriteria& criteria
	) = 0;
};

typedef std::shared_ptr<IWalletManager> IWalletManagerPtr;

namespace WalletAPI
{
	//
	// Creates a new instance of the Wallet manager.
	//
	WALLET_API IWalletManagerPtr CreateWalletManager(const Config& config, INodeClientPtr pNodeClient);
}