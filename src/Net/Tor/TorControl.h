#pragma once

#include <Crypto/Models/ed25519_secret_key.h>
#include <Net/Tor/TorAddress.h>
#include <Common/ChildProcess.h>
#include <filesystem.h>

// Forward Declarations
class TorControlClient;

class TorControl
{
public:
	TorControl(
		const uint16_t socksPort,
		const uint16_t controlPort,
		const fs::path& torDataPath,
		std::unique_ptr<TorControlClient>&& pClient,
		ChildProcess::UCPtr&& pProcess
	);
	~TorControl();

	static std::shared_ptr<TorControl> Create(
		const uint16_t socksPort,
		const uint16_t controlPort,
		const fs::path& torDataPath
	) noexcept;

	std::string AddOnion(const ed25519_secret_key_t& secretKey, const uint16_t externalPort, const uint16_t internalPort);
	bool DelOnion(const TorAddress& torAddress);

	bool CheckHeartbeat();
	std::vector<std::string> QueryHiddenServices();

private:
	uint16_t m_socksPort;
	uint16_t m_controlPort;
	fs::path m_torDataPath;
	std::unique_ptr<TorControlClient> m_pClient;
	ChildProcess::UCPtr m_pProcess;
};