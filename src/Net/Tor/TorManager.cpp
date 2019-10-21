#include <Net/Tor/TorManager.h>
#include <Net/Tor/TorException.h>
#include <Net/Tor/TorAddressParser.h>
#include <Infrastructure/Logger.h>
#include <memory>

#include "TorControl.h"

TorManager& TorManager::GetInstance(const TorConfig& config)
{
	static TorManager instance(config);
	return instance;
}

TorManager::TorManager(const TorConfig& config)
	: m_torConfig(config)
{
	m_pControl = new TorControl(config);
	m_pControl->Initialize();
}

TorManager::~TorManager()
{
	m_pControl->Shutdown();
	delete m_pControl;
}

std::unique_ptr<TorAddress> TorManager::AddListener(const SecretKey& privateKey, const int portNumber)
{
	try
	{
		const std::string address = m_pControl->AddOnion(privateKey, 80, portNumber);
		if (!address.empty())
		{
			std::optional<TorAddress> torAddress = TorAddressParser::Parse(address);
			if (!torAddress.has_value())
			{
				LOG_ERROR_F("Failed to parse listener address: %s", address.c_str());
			}
			else
			{
				return std::make_unique<TorAddress>(torAddress.value());
			}
		}
	}
	catch (const TorException& e)
	{
		LOG_ERROR_F("Failed to add listener: %s", e.what());
	}

	return nullptr;
}

bool TorManager::RemoveListener(const TorAddress& torAddress)
{
	try
	{
		return m_pControl->DelOnion(torAddress);
	}
	catch (const TorException& e)
	{
		LOG_ERROR_F("Failed to remove listener: %s", e.what());
	}

	return false;
}

std::unique_ptr<TorConnection> TorManager::Connect(const TorAddress& address)
{
	try
	{
		SocketAddress proxyAddress(IPAddress::FromString("127.0.0.1"), m_torConfig.GetSocksPort());

		return std::make_unique<TorConnection>(address, std::move(proxyAddress));
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Failed to create TorConnection: %s", e.what());
		return nullptr;
	}
}