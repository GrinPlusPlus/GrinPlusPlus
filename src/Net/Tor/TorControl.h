#pragma once

#include "TorControlClient.h"

#include <Crypto/SecretKey.h>
#include <Crypto/ED25519.h>
#include <Config/TorConfig.h>
#include <Net/Tor/TorAddress.h>
#include <Common/ChildProcess.h>

class TorControl
{
public:
	~TorControl() = default;

	static std::shared_ptr<TorControl> Create(const TorConfig& torConfig) noexcept;

	std::string AddOnion(const ed25519_secret_key_t& secretKey, const uint16_t externalPort, const uint16_t internalPort);
	std::string AddOnion(const std::string& serializedKey, const uint16_t externalPort, const uint16_t internalPort);
	bool DelOnion(const TorAddress& torAddress);

private:
	TorControl(
		const TorConfig& torConfig,
		std::shared_ptr<TorControlClient> pClient,
		ChildProcess::UCPtr&& pProcess
	);

	static bool Authenticate(std::shared_ptr<TorControlClient> pClient, const std::string& password);

	const TorConfig& m_torConfig;
	std::shared_ptr<TorControlClient> m_pClient;
	ChildProcess::UCPtr m_pProcess;
};