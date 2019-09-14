#include <catch.hpp>

#include "Helpers/TestNodeClient.h"

#include <Wallet/WalletManager.h>
#include <Config/ConfigManager.h>
#include <uuid.h>

TEST_CASE("Wallet Creation/Deletion")
{
	Config config = ConfigManager::LoadConfig(EEnvironmentType::MAINNET);
	TestNodeClient nodeClient;
	IWalletManager* pWalletManager = WalletAPI::StartWalletManager(config, nodeClient);

	// Create wallet
	const std::string username = uuids::to_string(uuids::uuid_system_generator()());
	std::optional<std::pair<SecureString, SessionToken>> wallet = pWalletManager->InitializeNewWallet(username, "Password1");
	REQUIRE(wallet.has_value());

	// Validate seed words
	SecureString seedWords = pWalletManager->GetSeedWords(wallet.value().second);
	REQUIRE(wallet.value().first == seedWords);

	// Logout
	pWalletManager->Logout(wallet.value().second);

	// Delete wallet
	const bool deleted = pWalletManager->DeleteWallet(username, "Password1");
	REQUIRE(deleted == true);
}