#pragma once

#include <Wallet/SessionToken.h>
#include <Net/Tor/TorProcess.h>
#include <API/Wallet/Foreign/ForeignServer.h>
#include <unordered_map>
#include <optional>
#include <string>
#include <mutex>

// Forward Declarations
class IWalletManager;

class ForeignController
{
public:
	ForeignController(IWalletManager& walletManager);
	~ForeignController();

	std::pair<uint16_t, std::optional<TorAddress>> StartListener(
		const TorProcess::Ptr& pTorProcess,
		const std::string& username,
		const SessionToken& token,
		const KeyChain& keyChain
	);
	bool StopListener(const std::string& username);

private:
	struct Context
	{
		Context(ForeignServer::UPtr&& pServer)
			: m_numReferences(1), m_pServer(std::move(pServer)) { }

		int m_numReferences;
		ForeignServer::UPtr m_pServer;
	};

	IWalletManager& m_walletManager;

	mutable std::mutex m_contextsMutex;
	std::unordered_map<std::string, std::unique_ptr<Context>> m_contextsByUsername;
};