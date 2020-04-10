#include "ForeignController.h"
#include <Wallet/Keychain/KeyChain.h>

#include <Infrastructure/Logger.h>
#include <Wallet/WalletManager.h>
#include <Net/Tor/TorManager.h>
#include <Net/Util/HTTPUtil.h>
#include <Net/Clients/RPC/RPC.h>

#include <API/Wallet/Foreign/Models/CheckVersionResponse.h>
#include <API/Wallet/Foreign/Models/ReceiveTxRequest.h>
#include <API/Wallet/Foreign/Models/ReceiveTxResponse.h>

ForeignController::ForeignController(const Config& config, IWalletManager& walletManager)
	: m_config(config), m_walletManager(walletManager)
{

}

ForeignController::~ForeignController()
{
	for (auto iter : m_contextsByUsername)
	{
		iter.second->m_pServer.reset();
	}
}

std::pair<uint16_t, std::optional<TorAddress>> ForeignController::StartListener(
	const std::string& username,
	const SessionToken& token,
	const SecureVector& seed)
{
	std::unique_lock<std::mutex> lock(m_contextsMutex);

	auto iter = m_contextsByUsername.find(username);
	if (iter != m_contextsByUsername.end())
	{
		iter->second->m_numReferences++;
		return std::make_pair(
			(uint16_t)iter->second->m_pServer->GetPortNumber(),
			iter->second->m_pServer->GetTorAddress()
		);
	}

	auto pServer = ForeignServer::Create(m_config, m_walletManager, token);
	auto torAddress = pServer->AddTorListener(seed);

	m_contextsByUsername[username] = std::make_shared<Context>(pServer);

	return std::make_pair(pServer->GetPortNumber(), torAddress);
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