#include <catch.hpp>

#include <TorProcessManager.h>
#include <TestServer.h>
#include <TestWalletServer.h>
#include <Crypto/RandomNumberGenerator.h>
#include <Net/Tor/TorConnection.h>

TEST_CASE("TorConnection - Multiple calls")
{
	TestServer::Ptr pTestServer = TestServer::Create();
	TestWalletServer::Ptr pTestWalletServer = TestWalletServer::Create(pTestServer);

	auto pSender = pTestWalletServer->CreateUser("Sender", "P@ssw0rd123!");
	REQUIRE(pSender->GetTorAddress().has_value());

	TorProcess::Ptr pTorProcess1 = TorProcessManager::GetProcess(1);
	auto pConnection1 = pTorProcess1->Connect(*pSender->GetTorAddress());

	auto request = RPC::Request::BuildRequest("check_version");
	auto response = pConnection1->Invoke(request, "/v2/foreign");

	REQUIRE(response.GetResult().has_value());

	auto response2 = pConnection1->Invoke(request, "/v2/foreign");
	REQUIRE(response2.GetResult().has_value());
}