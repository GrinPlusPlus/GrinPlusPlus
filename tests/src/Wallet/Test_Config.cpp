#include <filesystem>
#include <string>

#include <catch.hpp>

#include <Core/Enums/Environment.h>
#include <Core/Config.h>

TEST_CASE("Config")
{
	ConfigPtr pConfig = Config::Load(Environment::AUTOMATED_TESTING);
	
	REQUIRE(pConfig->GetMinimumConfirmations() == 10);
	REQUIRE(pConfig->GetMinPeers() == 10);
	REQUIRE(pConfig->GetMaxPeers() == 50);
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


	pConfig->SetMinConfirmations (25);
	pConfig->SetMinPeers(1);
	pConfig->SetMaxPeers(15);
	pConfig->ShouldReuseAddresses(false);

	REQUIRE(pConfig->GetMinimumConfirmations() == 25);
	REQUIRE(pConfig->GetMinPeers() == 1);
	REQUIRE(pConfig->GetMaxPeers() == 15);
	REQUIRE(pConfig->ShouldReuseAddresses() == false);
}