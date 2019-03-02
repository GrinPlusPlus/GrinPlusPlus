#include <ThirdParty/Catch2/catch.hpp>

#include <Wallet/WalletServer.h>
#include <Config/ConfigManager.h>
#include <uuid.h>

class TestNodeClient : public INodeClient
{
public:
	virtual uint64_t GetChainHeight() const override final { return 0; }
	virtual uint64_t GetNumConfirmations(const Commitment& outputCommitment) const override final { return 0; }
};

TEST_CASE("WalletServer::InitiailizeNewWallet")
{
	Config config = ConfigManager::LoadConfig();
	TestNodeClient nodeClient;
	IWalletServer* pWalletServer = WalletAPI::StartWalletServer(config, nodeClient);

	const std::string username = uuids::to_string(uuids::uuid_system_generator()());
	SecureString words = pWalletServer->InitializeNewWallet(username, "Password1");
	REQUIRE(!words.empty());
}