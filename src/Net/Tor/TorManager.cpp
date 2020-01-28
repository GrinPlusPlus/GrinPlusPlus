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
	if (m_torConfig.IsEnabled())
	{
		m_pControl = TorControl::Create(config);
	}
}

bool TorManager::RetryInit()
{
	if (m_pControl == nullptr)
	{
		m_pControl = TorControl::Create(m_torConfig);
	}

	return m_pControl != nullptr;
}

std::shared_ptr<TorAddress> TorManager::AddListener(const SecretKey64& secretKey, const uint16_t portNumber)
{
	try
	{
		if (m_pControl != nullptr)
		{
			const std::string address = m_pControl->AddOnion(secretKey, 80, portNumber);
			if (!address.empty())
			{
				std::optional<TorAddress> torAddress = TorAddressParser::Parse(address);
				if (!torAddress.has_value())
				{
					LOG_ERROR_F("Failed to parse listener address: {}", address);
				}
				else
				{
					return std::make_shared<TorAddress>(torAddress.value());
				}
			}
		}
	}
	catch (const TorException& e)
	{
		LOG_ERROR_F("Failed to add listener: {}", e.what());
	}

	return nullptr;
}

std::shared_ptr<TorAddress> TorManager::AddListener(const std::string& serializedKey, const uint16_t portNumber)
{
	try
	{
		if (m_pControl != nullptr)
		{
			const std::string address = m_pControl->AddOnion(serializedKey, 80, portNumber);
			if (!address.empty())
			{
				std::optional<TorAddress> torAddress = TorAddressParser::Parse(address);
				if (!torAddress.has_value())
				{
					LOG_ERROR_F("Failed to parse listener address: {}", address);
				}
				else
				{
					return std::make_shared<TorAddress>(torAddress.value());
				}
			}
		}
	}
	catch (const TorException& e)
	{
		LOG_ERROR_F("Failed to add listener: {}", e.what());
	}

	return nullptr;
}

bool TorManager::RemoveListener(const TorAddress& torAddress)
{
	try
	{
		if (m_pControl != nullptr)
		{
			return m_pControl->DelOnion(torAddress);
		}
	}
	catch (const TorException& e)
	{
		LOG_ERROR_F("Failed to remove listener: {}", e.what());
	}

	return false;
}

std::shared_ptr<TorConnection> TorManager::Connect(const TorAddress& address)
{
	if (!m_torConfig.IsEnabled())
	{
		return nullptr;
	}

	try
	{
		SocketAddress proxyAddress("127.0.0.1", m_torConfig.GetSocksPort());

		return std::make_shared<TorConnection>(address, std::move(proxyAddress));
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Failed to create TorConnection: {}", e.what());
		return nullptr;
	}
}