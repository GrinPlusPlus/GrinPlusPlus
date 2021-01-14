#include <Net/Tor/TorProcess.h>
#include <Net/Tor/TorException.h>
#include <Net/Tor/TorAddressParser.h>
#include <Net/Tor/TorConnection.h>
#include <Common/Util/ThreadUtil.h>
#include <Common/Logger.h>
#include <Crypto/ED25519.h>
#include <Core/Global.h>
#include <cstdlib>
#include <memory>

#include "TorControl.h"

TorProcess::~TorProcess()
{
	m_shutdown = true;
	LOG_INFO("Terminating tor process");
	ThreadUtil::Join(m_initThread);
	LOG_INFO("Terminated tor process");
}

TorProcess::Ptr TorProcess::Initialize(const fs::path& torDataPath, const uint16_t socksPort, const uint16_t controlPort) noexcept
{
	auto pProcess =  std::shared_ptr<TorProcess>(new TorProcess(torDataPath, socksPort, controlPort));

	pProcess->m_initThread = std::thread(TorProcess::Thread_Initialize, pProcess.get());
	return pProcess;
}

void TorProcess::Thread_Initialize(TorProcess* pProcess)
{
	while (Global::IsRunning() && !pProcess->m_shutdown)
	{
		try
		{
			std::unique_lock<std::mutex> lock(pProcess->m_mutex);
			if (pProcess->m_pControl != nullptr) {
				if (!pProcess->m_pControl->CheckHeartbeat()) {
					LOG_WARNING("Tor heartbeat failed. Killing process and reinitializing.");
					pProcess->m_pControl.reset();
					continue;
				}

				const size_t size = pProcess->m_servicesToAdd.size();
				std::vector<std::pair<ed25519_secret_key_t, uint16_t>> services_to_add =
					pProcess->m_servicesToAdd.copy_front(size);
				pProcess->m_servicesToAdd.pop_front(size);

				for (const auto& service : services_to_add) {
					auto pAdded = pProcess->AddListener(lock, service.first, service.second);
					if (pAdded == nullptr) {
						// Failed - Add it to back to retry.
						pProcess->m_servicesToAdd.push_back(service);
					}
				}

				lock.unlock();
				ThreadUtil::SleepFor(std::chrono::seconds(30));
				continue;
			}

			if (!IsPortOpen(pProcess->m_socksPort) || !IsPortOpen(pProcess->m_controlPort)) {
				LOG_WARNING("Tor port(s) in use. Trying to end tor process.");
#ifdef _WIN32
				system("taskkill /IM tor.exe /F");
#else
				system("killall tor");
#endif
				ThreadUtil::SleepFor(std::chrono::seconds(5));
				continue;
			}

			LOG_INFO("Initializing Tor");
			pProcess->m_pControl = TorControl::Create(
				pProcess->m_socksPort,
				pProcess->m_controlPort,
				pProcess->m_torDataPath
			);
			LOG_INFO_F("Tor Initialized: {}", pProcess->m_pControl != nullptr);

			auto addresses_to_add = pProcess->m_activeServices;
			lock.unlock();
			
			for (auto iter = addresses_to_add.cbegin(); iter != addresses_to_add.cend(); iter++)
			{
				auto pTorAddress = pProcess->AddListener(iter->second.first, iter->second.second);
			}

			ThreadUtil::SleepFor(std::chrono::seconds(10));
		}
		catch (const std::exception& e)
		{
			LOG_ERROR_F("Exception thrown: {}", e);
			ThreadUtil::SleepFor(std::chrono::seconds(30));
		}
	}
}

bool TorProcess::IsPortOpen(const uint16_t port)
{
    asio::io_service svc;
    asio::ip::tcp::acceptor a(svc);

    std::error_code ec;
    a.open(asio::ip::tcp::v4(), ec) || a.bind(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), port), ec);

    return ec != asio::error::address_in_use;
}

bool TorProcess::RetryInit()
{
	std::unique_lock<std::mutex> lock(m_mutex);

	if (m_pControl == nullptr) {
		m_pControl = TorControl::Create(m_socksPort, m_controlPort, m_torDataPath);
		return m_pControl != nullptr;
	}

	return false;
}

TorAddress TorProcess::AddListener(const ed25519_secret_key_t& secretKey, const uint16_t portNumber)
{
	m_servicesToAdd.push_back(std::make_pair(secretKey, portNumber));

	return TorAddressParser::FromPubKey(ED25519::CalculatePubKey(secretKey));
}

std::unique_ptr<TorAddress> TorProcess::AddListener(const std::unique_lock<std::mutex>&, const ed25519_secret_key_t& secretKey, const uint16_t portNumber)
{
	try {
		if (m_pControl != nullptr) {
			const std::string address = m_pControl->AddOnion(secretKey, 80, portNumber);
			if (!address.empty()) {
				std::optional<TorAddress> torAddress = TorAddressParser::Parse(address);
				if (!torAddress.has_value()) {
					LOG_ERROR_F("Failed to parse listener address: {}", address);
				} else {
					m_activeServices.insert({ address, { secretKey, portNumber } });
					return std::make_unique<TorAddress>(torAddress.value());
				}
			}
		}
	}
	catch (const TorException& e) {
		LOG_ERROR_F("Failed to add listener: {}", e.what());
	}

	return nullptr;
}

bool TorProcess::RemoveListener(const TorAddress& torAddress)
{
	std::unique_lock<std::mutex> lock(m_mutex);

	try
	{
		auto iter = m_activeServices.find(torAddress.ToString());
		if (iter != m_activeServices.end()) {
			m_activeServices.erase(iter);
		}

		if (m_pControl != nullptr) {
			return m_pControl->DelOnion(torAddress);
		}
	}
	catch (const TorException& e)
	{
		LOG_ERROR_F("Failed to remove listener: {}", e.what());
	}

	return false;
}

std::shared_ptr<ITorConnection> TorProcess::Connect(const TorAddress& address)
{
	std::unique_lock<std::mutex> lock(m_mutex);

	try
	{
		return std::make_shared<TorConnection>(address, SocketAddress{ "127.0.0.1", m_socksPort });
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Failed to create TorConnection: {}", e.what());
		return nullptr;
	}
}