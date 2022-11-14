#include <filesystem>
#include <string>

#include <catch.hpp>

#include <Core/Enums/Environment.h>
#include <Core/Config.h>


TEST_CASE("Config")
{
	ConfigPtr pConfig = Config::Default(Environment::AUTOMATED_TESTING);
	
	REQUIRE(pConfig->GetMinimumConfirmations() == 10);
	REQUIRE(pConfig->GetMinPeers() == 10);
	REQUIRE(pConfig->GetMaxPeers() == 60);
	REQUIRE(pConfig->ShouldReuseAddresses() == true);
	REQUIRE(pConfig->GetChainPath().generic_u8string().find("CHAIN") != std::string::npos);
	REQUIRE(pConfig->GetControlPassword() == "MyPassword");
	REQUIRE(pConfig->GetControlPort() == 3423);
	REQUIRE(pConfig->GetDatabasePath().generic_u8string().find("DB") != std::string::npos);
	REQUIRE(pConfig->GetDataDirectory().generic_u8string().find(".GrinPP") != std::string::npos);
	REQUIRE(pConfig->GetEmbargoSeconds() == 180);
	REQUIRE(pConfig->GetFeeBase() == 0x7a120);
	REQUIRE(pConfig->GetHashedControlPassword() == "16:906248AB51F939ED605CE9937D3B1FDE65DEB4098A889B2A07AC221D8F");
	REQUIRE(pConfig->GetLogDirectory().generic_u8string().find("LOGS") != std::string::npos);
	REQUIRE(pConfig->GetLogLevel() == "DEBUG");
	REQUIRE(pConfig->IsTorBridgesEnabled() == false);
	// TOR
	fs::path dataPath = pConfig->GetTorDataPath();
	std::string torrcPath{ pConfig->GetTorrcPath().u8string() };
	REQUIRE(torrcPath == (dataPath/".torrc").u8string());
	
	std::string bridge1 = "obfs4 185.177.207.6:63133 ADEF8A9E6AC0E564B9AE5C43AE8CE8B6C7006A85 cert = p9L6 + 25s8bnfkye1ZxFeAE4mAGY7DH4Gaj7dxngIIzP9BtqrHHwZXdjMK0RVIQ34C7aqZw iat - mode = 2";
	std::string bridge2 = "obfs4 186.177.207.7:63133 BDEF8A9E6AC0E564B9AE5C43AE8CE8B6C7006A85 cert = p9L6 + 25s8bnfkye1ZxFeAE4mAGY7DH4Gaj7dxngIIzP9BtqrHHwZXdjMK0RVIQ34C7aqZw iat - mode = 2";
	std::string bridge3 = "obfs4 187.177.207.8:63133 CDEF8A9E6AC0E564B9AE5C43AE8CE8B6C7006A85 cert = p9L6 + 25s8bnfkye1ZxFeAE4mAGY7DH4Gaj7dxngIIzP9BtqrHHwZXdjMK0RVIQ34C7aqZw iat - mode = 2";
	std::string bridge4 = "obfs4 188.177.207.9:63133 DDEF8A9E6AC0E564B9AE5C43AE8CE8B6C7006A85 cert = p9L6 + 25s8bnfkye1ZxFeAE4mAGY7DH4Gaj7dxngIIzP9BtqrHHwZXdjMK0RVIQ34C7aqZw iat - mode = 2";
	pConfig->AddObfs4TorBridge(bridge1);
	pConfig->AddObfs4TorBridge(bridge2);
	pConfig->AddObfs4TorBridge(bridge3);
	pConfig->AddObfs4TorBridge(bridge4);
	std::vector<std::string> bridges = pConfig->GetTorBridgesList();
	REQUIRE(bridges[0] == bridge1);
	REQUIRE(bridges[1] == bridge2);
	REQUIRE(bridges[2] == bridge3);
	REQUIRE(bridges[3] == bridge4);
	REQUIRE(std::any_of(bridges.cbegin(), bridges.cend(), [bridge1](std::string bridge) { return bridge == bridge1; }) == true);
	REQUIRE(std::any_of(bridges.cbegin(), bridges.cend(), [bridge2](std::string bridge) { return bridge == bridge2; }) == true);
	REQUIRE(std::any_of(bridges.cbegin(), bridges.cend(), [bridge3](std::string bridge) { return bridge == bridge3; }) == true);
	REQUIRE(std::any_of(bridges.cbegin(), bridges.cend(), [bridge4](std::string bridge) { return bridge == bridge4; }) == true);
	REQUIRE(pConfig->GetTorrcFileContent().find("UseBridges 1") != std::string::npos);
	REQUIRE(pConfig->GetTorrcFileContent().find("ClientTransportPlugin obfs4") != std::string::npos);
	REQUIRE(pConfig->GetTorrcFileContent().find(bridge1) != std::string::npos);
	REQUIRE(pConfig->GetTorrcFileContent().find(bridge2) != std::string::npos);
	REQUIRE(pConfig->GetTorrcFileContent().find(bridge3) != std::string::npos);
	REQUIRE(pConfig->GetTorrcFileContent().find(bridge4) != std::string::npos);
	REQUIRE(pConfig->IsTorBridgesEnabled() == true);
	REQUIRE(pConfig->IsObfs4Enabled() == true);
	REQUIRE(pConfig->IsSnowflakeEnabled() == false);
	REQUIRE(pConfig->DisableObfsBridges() == true);
	REQUIRE(pConfig->GetTorrcFileContent() == "");
	REQUIRE(pConfig->EnableSnowflake() == true);
	REQUIRE(pConfig->GetTorrcFileContent().find("UseBridges 1") != std::string::npos);
	REQUIRE(pConfig->GetTorrcFileContent().find("ClientTransportPlugin snowflake") != std::string::npos);
	REQUIRE(pConfig->GetTorrcFileContent().find("Bridge snowflake") != std::string::npos);
	REQUIRE(pConfig->IsTorBridgesEnabled() == true);
	REQUIRE(pConfig->IsObfs4Enabled() == false);
	REQUIRE(pConfig->IsSnowflakeEnabled() == true);
	REQUIRE(pConfig->DisableSnowflake() == true);
	REQUIRE(pConfig->IsSnowflakeEnabled() == false);
	REQUIRE(pConfig->IsTorBridgesEnabled() == false);
	REQUIRE(pConfig->GetTorrcFileContent() == "");
	
	pConfig->SetMinConfirmations (25);
	pConfig->SetMinPeers(1);
	pConfig->SetMaxPeers(15);
	pConfig->ShouldReuseAddresses(false);

	REQUIRE(pConfig->GetMinimumConfirmations() == 25);
	REQUIRE(pConfig->GetMinPeers() == 1);
	REQUIRE(pConfig->GetMaxPeers() == 15);
	REQUIRE(pConfig->ShouldReuseAddresses() == false);

	std::unordered_set<IPAddress> preferredPeers = { IPAddress::Parse("10.0.0.10"), IPAddress::Parse("10.0.0.11"), IPAddress::Parse("10.0.0.12") };
	std::unordered_set<IPAddress> allowedPeers = { IPAddress::Parse("10.0.0.13"), IPAddress::Parse("10.0.0.14") };
	std::unordered_set<IPAddress> blockedPeers = { IPAddress::Parse("10.0.0.11") };

	pConfig->UpdatePreferredPeers(preferredPeers);
	pConfig->UpdateAllowedPeers(allowedPeers);
	pConfig->UpdateBlockedPeers(blockedPeers);

	REQUIRE(pConfig->GetPreferredPeers() == preferredPeers);
	REQUIRE(pConfig->GetAllowedPeers() == allowedPeers);
	REQUIRE(pConfig->GetBlockedPeers() == blockedPeers);

	pConfig->UpdatePreferredPeers({});
	pConfig->UpdateAllowedPeers({});
	pConfig->UpdateBlockedPeers({});

	REQUIRE(pConfig->GetPreferredPeers().empty() == true);
	REQUIRE(pConfig->GetAllowedPeers().empty() == true);
	REQUIRE(pConfig->GetBlockedPeers().empty() == true);
}