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

	// TOR
	fs::path dataPath = pConfig->GetTorDataPath();
	std::string torrcPath{ pConfig->GetTorrcPath().u8string() };
	REQUIRE(torrcPath == (dataPath/".torrc").u8string());
	
	std::string bridge1 = "obfs4 185.177.207.6:63133 ADEF8A9E6AC0E564B9AE5C43AE8CE8B6C7006A85 cert = p9L6 + 25s8bnfkye1ZxFeAE4mAGY7DH4Gaj7dxngIIzP9BtqrHHwZXdjMK0RVIQ34C7aqZw iat - mode = 2";
	std::string bridge2 = "obfs4 185.177.207.7:63133 BDEF8A9E6AC0E564B9AE5C43AE8CE8B6C7006A85 cert = p9L6 + 25s8bnfkye1ZxFeAE4mAGY7DH4Gaj7dxngIIzP9BtqrHHwZXdjMK0RVIQ34C7aqZw iat - mode = 2";
	std::string bridge3 = "obfs4 185.177.207.8:63133 CDEF8A9E6AC0E564B9AE5C43AE8CE8B6C7006A85 cert = p9L6 + 25s8bnfkye1ZxFeAE4mAGY7DH4Gaj7dxngIIzP9BtqrHHwZXdjMK0RVIQ34C7aqZw iat - mode = 2";
	std::string bridge4 = "obfs4 185.177.207.9:63133 DDEF8A9E6AC0E564B9AE5C43AE8CE8B6C7006A85 cert = p9L6 + 25s8bnfkye1ZxFeAE4mAGY7DH4Gaj7dxngIIzP9BtqrHHwZXdjMK0RVIQ34C7aqZw iat - mode = 2";
	pConfig->AddTorBridge(bridge1);
	pConfig->AddTorBridge(bridge2);
	pConfig->AddTorBridge(bridge3);
	pConfig->AddTorBridge(bridge4);
	REQUIRE(pConfig->ReadTorrcFile().find("UseBridges 1") != std::string::npos);
	REQUIRE(pConfig->ReadTorrcFile().find("ClientTransportPlugin") != std::string::npos);
	REQUIRE(pConfig->ReadTorrcFile().find(bridge1) != std::string::npos);
	REQUIRE(pConfig->ReadTorrcFile().find(bridge2) != std::string::npos);
	REQUIRE(pConfig->ReadTorrcFile().find(bridge3) != std::string::npos);
	REQUIRE(pConfig->ReadTorrcFile().find(bridge4) != std::string::npos);
	pConfig->ClearTorrcFile();
	REQUIRE(pConfig->ReadTorrcFile() == "");
	
	
	pConfig->SetMinConfirmations (25);
	pConfig->SetMinPeers(1);
	pConfig->SetMaxPeers(15);
	pConfig->ShouldReuseAddresses(false);

	REQUIRE(pConfig->GetMinimumConfirmations() == 25);
	REQUIRE(pConfig->GetMinPeers() == 1);
	REQUIRE(pConfig->GetMaxPeers() == 15);
	REQUIRE(pConfig->ShouldReuseAddresses() == false);
}