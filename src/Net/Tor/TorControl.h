#pragma once

#include <Crypto/Models/ed25519_secret_key.h>
#include <Config/TorConfig.h>
#include <Net/Tor/TorAddress.h>
#include <Common/ChildProcess.h>

// Forward Declarations
class TorControlClient;

class TorControl
{
public:
	TorControl(
		const TorConfig& torConfig,
		std::unique_ptr<TorControlClient>&& pClient,
		ChildProcess::UCPtr&& pProcess
	);
	~TorControl();

	static std::shared_ptr<TorControl> Create(const TorConfig& torConfig) noexcept;

	std::string AddOnion(const ed25519_secret_key_t& secretKey, const uint16_t externalPort, const uint16_t internalPort);
	bool DelOnion(const TorAddress& torAddress);

	bool CheckHeartbeat();
	std::vector<std::string> QueryHiddenServices();

private:
	const TorConfig& m_torConfig;
	std::unique_ptr<TorControlClient> m_pClient;
	ChildProcess::UCPtr m_pProcess;
};