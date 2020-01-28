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

	std::shared_ptr<TorAddress> AddListener(const SecretKey64& secretKey, const uint16_t portNumber);
	std::shared_ptr<TorAddress> AddListener(const std::string& serializedKey, const uint16_t portNumber);
	bool RemoveListener(const TorAddress& torAddress);

	std::shared_ptr<TorConnection> Connect(const TorAddress& address);

	// Returns true if TorControl connection is established.
	bool RetryInit();

private:
	TorManager(const TorConfig& config);

	const TorConfig& m_torConfig;
	std::shared_ptr<TorControl> m_pControl;
	asio::io_context m_context;
};