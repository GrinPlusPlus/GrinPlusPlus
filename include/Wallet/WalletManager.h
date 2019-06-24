#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Common/ImportExport.h>
#include <Config/Config.h>
#include <Wallet/NodeClient.h>
#include <Wallet/SessionToken.h>
#include <Wallet/SelectionStrategy.h>
#include <Wallet/Slate.h>
#include <Wallet/WalletSummary.h>
#include <Wallet/WalletTx.h>
#include <Wallet/Models/DTOs/WalletTxDTO.h>
#include <Common/Secure.h>
#include <Crypto/SecretKey.h>

#ifdef MW_WALLET
#define WALLET_API EXPORT
#else
#define WALLET_API IMPORT
#endif

class IWalletManager
{
public:
	//
	// Creates a new wallet with the username and password given, and returns the space-delimited wallet words (BIP39 mnemonics).
	// If a wallet for the user already exists, an empty string will be returned.
	//
	virtual std::optional<std::pair<SecureString, SessionToken>> InitializeNewWallet(const std::string& username, const SecureString& password) = 0;

	//
	// Creates a wallet from existing wallet words (BIP39 mnemonics).
	// Restoring outputs must be done separately.
	//
	virtual std::optional<SessionToken> RestoreFromSeed(const std::string& username, const SecureString& password, const SecureString& walletWords) = 0;

	// TODO: Determine return value.
	virtual bool CheckForOutputs(const SessionToken& token, const bool fromGenesis) = 0;

	virtual std::vector<std::string> GetAllAccounts() const = 0;

	virtual SecretKey GetGrinboxAddress(const SessionToken& token) const = 0;

	//
	// Authenticates the user, and if successful, returns a session token that can be used in lieu of credentials for future calls.
	//
	virtual std::unique_ptr<SessionToken> Login(const std::string& username, const SecureString& password) = 0;

	//
	// Deletes the session information.
	//
	virtual void Logout(const SessionToken& token) = 0;

	virtual WalletSummary GetWalletSummary(const SessionToken& token) = 0;

	virtual std::vector<WalletTxDTO> GetTransactions(const SessionToken& token) = 0;

	virtual std::vector<WalletOutputDTO> GetOutputs(const SessionToken& token, const bool includeSpent, const bool includeCanceled) = 0;

	virtual uint64_t EstimateFee(const SessionToken& token, const uint64_t amountToSend, const uint64_t feeBase, const ESelectionStrategy& strategy, const uint8_t numChangeOutputs) = 0;

	//
	// Sends coins to the given destination using the specified method. Returns a valid slate if successful.
	// Exceptions thrown:
	// * SessionTokenException - If no matching session found, or if the token is invalid.
	// * InsufficientFundsException - If there are not enough funds ready to spend after calculating and including the fee.
	//
	virtual std::unique_ptr<Slate> Send(const SessionToken& token, const uint64_t amount, const uint64_t feeBase, const std::optional<std::string>& messageOpt, const ESelectionStrategy& strategy, const uint8_t numChangeOutputs) = 0;

	virtual std::unique_ptr<Slate> Receive(const SessionToken& token, const Slate& slate, const std::optional<std::string>& messageOpt) = 0;

	virtual std::unique_ptr<Slate> Finalize(const SessionToken& token, const Slate& slate) = 0;

	virtual bool PostTransaction(const SessionToken& token, const Transaction& transaction) = 0;

	virtual bool RepostByTxId(const SessionToken& token, const uint32_t walletTxId) = 0;

	virtual bool CancelByTxId(const SessionToken& token, const uint32_t walletTxId) = 0;
};

namespace WalletAPI
{
	//
	// Creates a new instance of the Wallet server.
	//
	WALLET_API IWalletManager* StartWalletManager(const Config& config, INodeClient& nodeClient);

	//
	// Stops the Wallet server and clears up its memory usage.
	//
	WALLET_API void ShutdownWalletManager(IWalletManager* pWalletManager);
}