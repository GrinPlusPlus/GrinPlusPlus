#pragma once

#include <Config/TorConfig.h>
#include <Crypto/SecretKey.h>
#include <Net/Tor/TorAddress.h>
#include <Net/Tor/TorConnection.h>

// Forward Declarations
class TorControl;

class TorManager
{
public:
	static TorManager& GetInstance(const TorConfig& config);

	std::shared_ptr<TorAddress> AddListener(const SecretKey& privateKey, const int portNumber);
	bool RemoveListener(const TorAddress& torAddress);

	std::shared_ptr<TorConnection> Connect(const TorAddress& address);

private:
	TorManager(const TorConfig& config);

	const TorConfig& m_torConfig;
	std::shared_ptr<TorControl> m_pControl;
	asio::io_context m_context;
};