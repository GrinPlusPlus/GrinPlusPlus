#pragma once

#include <Config/ConfigProps.h>
#include <Config/TorConfig.h>
#include <Config/EnvironmentType.h>
#include <Common/Util/BitUtil.h>
#include <Common/Util/FileUtil.h>
#include <json/json.h>
#include <cstdint>
#include <string>

class WalletConfig
{
public:
	WalletConfig(const Json::Value& json, const EEnvironmentType environment, const fs::path& dataPath)
	{
		if (environment == EEnvironmentType::MAINNET)
		{
			m_listenPort = 3415; // TODO: Read value
			m_ownerPort = 3420; // TODO: Read value
			m_privateKeyVersion = BitUtil::ConvertToU32(0x03, 0x3C, 0x04, 0xA4);
			m_publicKeyVersion = BitUtil::ConvertToU32(0x03, 0x3C, 0x08, 0xDF);
		}
		else
		{
			m_listenPort = 13415;
			m_ownerPort = 13420;
			m_privateKeyVersion = BitUtil::ConvertToU32(0x03, 0x27, 0x3A, 0x10);
			m_publicKeyVersion = BitUtil::ConvertToU32(0x03, 0x27, 0x3E, 0x4B);
		}

		m_walletPath = FileUtil::ToPath(dataPath.u8string() + "WALLET/");
		FileUtil::CreateDirectories(m_walletPath);

		m_databaseType = "SQLITE";
		m_minimumConfirmations = 10;
		if (json.isMember(ConfigProps::Wallet::WALLET))
		{
			const Json::Value& walletJSON = json[ConfigProps::Wallet::WALLET];
			if (walletJSON.isMember(ConfigProps::Wallet::DATABASE))
			{
				m_databaseType = walletJSON.get(ConfigProps::Wallet::DATABASE, "SQLITE").asString();
			}
			if (walletJSON.isMember(ConfigProps::Wallet::MIN_CONFIRMATIONS))
			{
				m_minimumConfirmations = walletJSON.get(ConfigProps::Wallet::MIN_CONFIRMATIONS, 10).asUInt();
			}
		}
	}

	const fs::path& GetWalletDirectory() const { return m_walletPath; }
	const std::string& GetDatabaseType() const { return m_databaseType; }
	uint32_t GetListenPort() const { return m_listenPort; }
	uint32_t GetOwnerPort() const { return m_ownerPort; }
	uint32_t GetPublicKeyVersion() const { return m_publicKeyVersion; }
	uint32_t GetPrivateKeyVersion() const { return m_privateKeyVersion; }
	uint32_t GetMinimumConfirmations() const { return m_minimumConfirmations; }

private:
	fs::path m_walletPath;
	std::string m_databaseType;
	uint32_t m_listenPort;
	uint32_t m_ownerPort;
	uint32_t m_publicKeyVersion;
	uint32_t m_privateKeyVersion;
	uint32_t m_minimumConfirmations;
};
