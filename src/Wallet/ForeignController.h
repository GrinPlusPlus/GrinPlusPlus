#pragma once

#include <Net/Tor/TorProcess.h>
#include <unordered_map>
#include <optional>
#include <string>
#include <mutex>

// Forward Declarations
class IWalletManager;
class SessionToken;
class KeyChain;

class ForeignController
{
public:
	ForeignController(IWalletManager& walletManager);
	~ForeignController();

	std::pair<uint16_t, std::optional<TorAddress>> StartListener(
		const TorProcess::Ptr& pTorProcess,
		const std::string& username,
		const SessionToken& token,
		const KeyChain& keyChain,
		const int currentAddressIndex
	);
	bool StopListener(const std::string& username);

	bool IsListening(const std::string& username);

private:
	struct Context;

	IWalletManager& m_walletManager;

	mutable std::mutex m_contextsMutex;
	std::unordered_map<std::string, std::unique_ptr<Context>> m_contextsByUsername;
};