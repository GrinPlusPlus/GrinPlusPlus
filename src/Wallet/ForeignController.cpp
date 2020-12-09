#include "ForeignController.h"

#include <API/Wallet/Foreign/ForeignServer.h>
#include <Wallet/SessionToken.h>
#include <Wallet/Keychain/KeyChain.h>
#include <Wallet/WalletManager.h>

struct ForeignController::Context
{
	Context(ForeignServer::UPtr&& pServer)
		: m_numReferences(1), m_pServer(std::move(pServer)) { }

	int m_numReferences;
	ForeignServer::UPtr m_pServer;
};

ForeignController::ForeignController(IWalletManager& walletManager)
	: m_walletManager(walletManager)
{

}

ForeignController::~ForeignController()
{
	for (auto& iter : m_contextsByUsername)
	{
		iter.second->m_pServer.reset();
	}
}

std::pair<uint16_t, std::optional<TorAddress>> ForeignController::StartListener(
	const TorProcess::Ptr& pTorProcess,
	const std::string& username,
	const SessionToken& token,
	const KeyChain& keyChain)
{
	WALLET_INFO("Starting a listener...");
	std::unique_lock<std::mutex> lock(m_contextsMutex);

	WALLET_INFO("Getting user for listener...");
	auto iter = m_contextsByUsername.find(username);
	if (iter != m_contextsByUsername.end())
	{
		iter->second->m_numReferences++;
		return std::make_pair(
			(uint16_t)iter->second->m_pServer->GetPortNumber(),
			iter->second->m_pServer->GetTorAddress()
		);
	}

	WALLET_INFO("Trying to create the ForeignServer...");
	auto pServer = ForeignServer::Create(
		keyChain,
		pTorProcess,
		m_walletManager,
		token
	);

	WALLET_INFO_F("ForeignServer created on port: '{}'...", pServer->GetPortNumber());
	WALLET_INFO_F("ForeignServer created with address: '{}'...", pServer->GetTorAddress()->GetPublicKey());

	auto response = std::make_pair(pServer->GetPortNumber(), pServer->GetTorAddress());

	m_contextsByUsername[username] = std::make_unique<Context>(std::move(pServer));

	WALLET_INFO("Returning from ForeignController::StartListener");
	return response;
}

bool ForeignController::StopListener(const std::string& username)
{
	std::unique_lock<std::mutex> lock(m_contextsMutex);

	auto iter = m_contextsByUsername.find(username);
	if (iter == m_contextsByUsername.end())
	{
		return false;
	}

	iter->second->m_numReferences--;
	if (iter->second->m_numReferences == 0)
	{
		m_contextsByUsername.erase(iter);
	}

	return true;
}

bool ForeignController::IsListening(const std::string& username)
{
	std::unique_lock<std::mutex> lock(m_contextsMutex);

	return m_contextsByUsername.find(username) != m_contextsByUsername.end();
}