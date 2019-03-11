#pragma once

#include <stdint.h>
#include <string>

class WalletConfig
{
public:
	WalletConfig(const std::string& walletDirectory, const uint32_t listenPort, const uint32_t ownerPort, const uint32_t publicKeyVersion, const uint32_t privateKeyVersion, const uint32_t minimumConfirmations)
		: m_walletDirectory(walletDirectory), m_listenPort(listenPort), m_ownerPort(ownerPort), m_publicKeyVersion(publicKeyVersion), m_privateKeyVersion(privateKeyVersion), m_minimumConfirmations(minimumConfirmations)
	{

	}

	inline const std::string& GetWalletDirectory() const { return m_walletDirectory; }
	inline uint32_t GetListenPort() const { return m_listenPort; }
	inline uint32_t GetOwnerPort() const { return m_ownerPort; }
	inline uint32_t GetPublicKeyVersion() const { return m_publicKeyVersion; }
	inline uint32_t GetPrivateKeyVersion() const { return m_privateKeyVersion; }
	inline uint32_t GetMinimumConfirmations() const { return m_minimumConfirmations; }

private:
	std::string m_walletDirectory;
	uint32_t m_listenPort;
	uint32_t m_ownerPort;
	uint32_t m_publicKeyVersion;
	uint32_t m_privateKeyVersion;
	uint32_t m_minimumConfirmations;
};