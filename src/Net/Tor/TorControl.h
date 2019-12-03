#pragma once

#include "TorControlClient.h"

#include <Crypto/SecretKey.h>
#include <Config/TorConfig.h>
#include <Net/Tor/TorAddress.h>

class TorControl
{
public:
	~TorControl();

	static std::shared_ptr<TorControl> Create(const TorConfig& torConfig);

	std::string AddOnion(const SecretKey& privateKey, const uint16_t externalPort, const uint16_t internalPort);
	std::string AddOnion(const std::string& serializedKey, const uint16_t externalPort, const uint16_t internalPort);
	bool DelOnion(const TorAddress& torAddress);

private:
	TorControl(
		const TorConfig& torConfig,
		std::shared_ptr<TorControlClient> pClient,
		long processId
	);

	static bool Authenticate(std::shared_ptr<TorControlClient> pClient, const std::string& password);
	
	std::string FormatKey(const SecretKey& privateKey) const;

	const TorConfig& m_torConfig;
	std::shared_ptr<TorControlClient> m_pClient;
	long m_processId;
};