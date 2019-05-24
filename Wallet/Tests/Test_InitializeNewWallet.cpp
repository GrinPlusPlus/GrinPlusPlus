#include <ThirdParty/Catch2/catch.hpp>

#include "Helpers/TestNodeClient.h"

#include <Wallet/WalletManager.h>
#include <Config/ConfigManager.h>
#include <uuid.h>

TEST_CASE("WalletServer::InitiailizeNewWallet")
{
	Config config = ConfigManager::LoadConfig(EEnvironmentType::MAINNET);
	TestNodeClient nodeClient;
	IWalletManager* pWalletManager = WalletAPI::StartWalletManager(config, nodeClient);

	const std::string username = uuids::to_string(uuids::uuid_system_generator()());
	std::optional<std::pair<SecureString, SessionToken>> words = pWalletManager->InitializeNewWallet(username, "Password1");
	REQUIRE(words.has_value());
}