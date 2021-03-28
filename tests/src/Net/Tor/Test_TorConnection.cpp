#include <catch.hpp>

#include <TorProcessManager.h>
#include <TestServer.h>
#include <Crypto/CSPRNG.h>
#include <Net/Tor/TorConnection.h>

TEST_CASE("TorConnection - Multiple calls")
{
	TestServer::Ptr pTestServer = TestServer::CreateWithWallet();

	auto created_user = pTestServer->CreateUser("Sender", "P@ssw0rd123!");
	REQUIRE(created_user.wallet->GetTorAddress().has_value());

	TorProcess::Ptr pTorProcess1 = TorProcessManager::GetProcess(1);
	auto pConnection1 = pTorProcess1->Connect(created_user.wallet->GetTorAddress().value());

	auto request = RPC::Request::BuildRequest("check_version");
	auto response = pConnection1->Invoke(request, "/v2/foreign");

	REQUIRE(response.GetResult().has_value());

	auto response2 = pConnection1->Invoke(request, "/v2/foreign");
	REQUIRE(response2.GetResult().has_value());
}