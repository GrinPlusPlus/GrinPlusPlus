#pragma once

#include "ConfigProps.h"

#include <Core/Enums/Environment.h>
#include <Common/Util/BitUtil.h>
#include <Common/Util/FileUtil.h>
#include <json/json.h>
#include <cstdint>
#include <string>

class WalletConfig
{
public:
	WalletConfig(const Json::Value& json, const Environment environment, const fs::path& dataPath)
	{
		if (environment == Environment::MAINNET)
		{
			m_ownerPort = 3420; // TODO: Read value
			m_privateKeyVersion = BitUtil::ConvertToU32(0x03, 0x3C, 0x04, 0xA4);
			m_publicKeyVersion = BitUtil::ConvertToU32(0x03, 0x3C, 0x08, 0xDF);
		}
		else
		{
			m_ownerPort = 13420;
			m_privateKeyVersion = BitUtil::ConvertToU32(0x03, 0x27, 0x3A, 0x10);
			m_publicKeyVersion = BitUtil::ConvertToU32(0x03, 0x27, 0x3E, 0x4B);
		}

		m_walletPath = dataPath / "WALLET";
		FileUtil::CreateDirectories(m_walletPath);

		m_minimumConfirmations = 10;
		if (json.isMember(ConfigProps::Wallet::WALLET))
		{
			const Json::Value& walletJSON = json[ConfigProps::Wallet::WALLET];

			m_minimumConfirmations = walletJSON.get(ConfigProps::Wallet::MIN_CONFIRMATIONS, 10).asUInt();

			m_reuseAddress = walletJSON.get(ConfigProps::Wallet::REUSE_ADDRESS, 1).asUInt();
		}
	}

	const fs::path& GetWalletPath() const { return m_walletPath; }
	uint32_t GetOwnerPort() const { return m_ownerPort; }
	uint32_t GetPublicKeyVersion() const { return m_publicKeyVersion; }
	uint32_t GetPrivateKeyVersion() const { return m_privateKeyVersion; }
	uint32_t GetMinimumConfirmations() const { return m_minimumConfirmations; }
	void SetMinConfirmations(const uint32_t min_confirmations) { m_minimumConfirmations = min_confirmations; }
	uint32_t GetReuseAddress() const { return m_reuseAddress; }
	void SetReuseAddress(const uint32_t reuse_address) { m_reuseAddress = reuse_address; }

private:
	fs::path m_walletPath;
	uint32_t m_ownerPort;
	uint32_t m_publicKeyVersion;
	uint32_t m_privateKeyVersion;
	uint32_t m_minimumConfirmations;
	uint32_t m_reuseAddress;
};
