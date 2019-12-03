#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Common/ImportExport.h>
#include <Config/Config.h>
#include <Wallet/NodeClient.h>
#include <Wallet/SessionToken.h>
#include <Wallet/Enums/SelectionStrategy.h>
#include <Wallet/Slate.h>
#include <Wallet/WalletSummary.h>
#include <Wallet/WalletTx.h>
#include <Wallet/Models/Criteria/SendCriteria.h>
#include <Wallet/Models/Criteria/ReceiveCriteria.h>
#include <Wallet/Models/Criteria/FinalizeCriteria.h>
#include <Wallet/Models/DTOs/WalletTxDTO.h>
#include <Wallet/Models/DTOs/FeeEstimateDTO.h>
#include <Wallet/Models/DTOs/SelectionStrategyDTO.h>
#include <Net/Tor/TorAddress.h>
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
	virtual std::pair<SecureString, SessionToken> InitializeNewWallet(
		const std::string& username,
		const SecureString& password
	) = 0;

	//
	// Creates a wallet from existing wallet words (BIP39 mnemonics).
	// Restoring outputs must be done separately.
	//
	virtual std::optional<SessionToken> RestoreFromSeed(
		const std::string& username,
		const SecureString& password,
		const SecureString& walletWords
	) = 0;

	//
	// Returns the logged in user's wallet words.
	//
	virtual SecureString GetSeedWords(const SessionToken& token) = 0;

	virtual void CheckForOutputs(
		const SessionToken& token,
		const bool fromGenesis
	) = 0;

	virtual std::vector<std::string> GetAllAccounts() const = 0;

	virtual SecretKey GetGrinboxAddress(const SessionToken& token) const = 0;

	virtual std::optional<TorAddress> GetTorAddress(const SessionToken& token) = 0;

	//
	// Authenticates the user, and if successful, returns a session token that can be used in lieu of credentials for future calls.
	//
	virtual SessionToken Login(
		const std::string& username,
		const SecureString& password
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

	virtual WalletSummary GetWalletSummary(const SessionToken& token) = 0;

	virtual std::vector<WalletTxDTO> GetTransactions(const SessionToken& token) = 0;

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

	virtual Slate Finalize(const FinalizeCriteria& finalizeCriteria) = 0;

	virtual bool PostTransaction(
		const SessionToken& token,
		const Transaction& transaction,
		const PostMethodDTO& postMethod
	) = 0;

	virtual bool RepostByTxId(
		const SessionToken& token,
		const uint32_t walletTxId
	) = 0;

	virtual bool CancelByTxId(
		const SessionToken& token,
		const uint32_t walletTxId
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