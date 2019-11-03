#pragma once

#include <Config/Config.h>
#include <Wallet/SessionToken.h>
#include <Net/Tor/TorAddress.h>
#include <civetweb.h>
#include <unordered_map>
#include <optional>
#include <string>
#include <mutex>

// Forward Declarations
class IWalletManager;

class ForeignController
{
public:
	ForeignController(const Config& config, IWalletManager& walletManager);
	~ForeignController();

	std::optional<TorAddress> StartListener(const std::string& username, const SessionToken& token, const SecureVector& seed);
	bool StopListener(const std::string& username);

private:
	struct Context
	{
		Context(mg_context* pCivetContext, IWalletManager& walletManager, int portNumber, const SessionToken& token)
			: m_numReferences(1), m_walletManager(walletManager), m_pCivetContext(pCivetContext), m_portNumber(portNumber), m_token(token)
		{

		}

		int m_numReferences;
		IWalletManager& m_walletManager;
		mg_context* m_pCivetContext;
		SessionToken m_token;
		int m_portNumber;
		std::optional<TorAddress> m_torAddress;
	};

	static int ForeignAPIHandler(mg_connection* pConnection, void* pCbContext);

	const Config& m_config;
	IWalletManager& m_walletManager;

	mutable std::mutex m_contextsMutex;
	std::unordered_map<std::string, std::shared_ptr<Context>> m_contextsByUsername;
};