#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <ImportExport.h>
#include <Config/Config.h>
#include <Wallet/NodeClient.h>
#include <Common/SecureString.h>

#ifdef MW_WALLET
#define WALLET_API EXPORT
#else
#define WALLET_API IMPORT
#endif

class IWalletServer
{
public:
	//
	// Creates a new wallet with the username and password given, and returns the space-delimited wallet words (BIP39 mnemonics).
	// If a wallet for the user already exists, an empty string will be returned.
	//
	virtual SecureString InitializeNewWallet(const std::string& username, const SecureString& password) = 0;

	//
	// Authenticates the user, and if successful, stores the credentials to avoid having to pass them in for future calls.
	// Logging in makes consumption of other wallet server functions faster and more convenient, at the cost of your credentials being stored in memory.
	// USE AT YOUR OWN RISK!
	//
	virtual bool Login(const std::string& username, const SecureString& password) = 0;

	//
	// Deletes any credentials stored in memory.
	//
	virtual void Logoff() = 0;
};

namespace WalletAPI
{
	//
	// Creates a new instance of the Wallet server.
	//
	WALLET_API IWalletServer* StartWalletServer(const Config& config, const INodeClient& nodeClient);

	//
	// Stops the Wallet server and clears up its memory usage.
	//
	WALLET_API void ShutdownWalletServer(IWalletServer* pWalletServer);
}