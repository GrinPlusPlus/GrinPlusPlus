#include <Net/Tor/TorProcess.h>
#include <Net/Tor/TorException.h>
#include <Net/Tor/TorAddressParser.h>
#include <Net/Tor/TorConnection.h>
#include <Common/Util/ThreadUtil.h>
#include <Common/ShutdownManager.h>
#include <Common/Logger.h>
#include <cstdlib>
#include <memory>

#include "TorControl.h"

TorProcess::~TorProcess()
{
	LOG_INFO("Terminating tor process");
	ThreadUtil::Join(m_initThread);
}

TorProcess::Ptr TorProcess::Initialize(const fs::path& torDataPath, const uint16_t socksPort, const uint16_t controlPort) noexcept
{
	auto pProcess =  std::shared_ptr<TorProcess>(new TorProcess(torDataPath, socksPort, controlPort));

	pProcess->m_initThread = std::thread(TorProcess::Thread_Initialize, pProcess.get());
	return pProcess;
}

void TorProcess::Thread_Initialize(TorProcess* pProcess)
{
	std::unique_lock<std::mutex> lock(pProcess->m_mutex);

	try
	{
		if (!IsPortOpen(pProcess->m_socksPort) || !IsPortOpen(pProcess->m_controlPort)) {
			LOG_WARNING("Tor port(s) in use. Trying to end tor process.");
#ifdef _WIN32
    		system("taskkill /IM tor.exe /F");
#else
			system("killall tor");
#endif
			ThreadUtil::SleepFor(std::chrono::seconds(5), ShutdownManagerAPI::WasShutdownRequested());
		}

		LOG_INFO("Initializing Tor");
		TorConfig config{ pProcess->m_socksPort, pProcess->m_controlPort, pProcess->m_torDataPath };
		pProcess->m_pControl = TorControl::Create(config);
		LOG_INFO_F("Tor Initialized: {}", pProcess->m_pControl != nullptr);
	}
	catch (const std::exception& e)
	{
		LOG_ERROR_F("Exception thrown: {}", e);
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
		m_pControl = TorControl::Create(TorConfig{ m_socksPort, m_controlPort, m_torDataPath });
	}

	return m_pControl != nullptr;
}

std::shared_ptr<TorAddress> TorProcess::AddListener(const ed25519_secret_key_t& secretKey, const uint16_t portNumber)
{
	std::unique_lock<std::mutex> lock(m_mutex);

	try
	{
		if (m_pControl != nullptr) {
			const std::string address = m_pControl->AddOnion(secretKey, 80, portNumber);
			if (!address.empty()) {
				std::optional<TorAddress> torAddress = TorAddressParser::Parse(address);
				if (!torAddress.has_value()) {
					LOG_ERROR_F("Failed to parse listener address: {}", address);
				} else {
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

std::shared_ptr<TorAddress> TorProcess::AddListener(const std::string& serializedKey, const uint16_t portNumber)
{
	std::unique_lock<std::mutex> lock(m_mutex);

	try
	{
		if (m_pControl != nullptr) {
			const std::string address = m_pControl->AddOnion(serializedKey, 80, portNumber);
			if (!address.empty()) {
				std::optional<TorAddress> torAddress = TorAddressParser::Parse(address);
				if (!torAddress.has_value()) {
					LOG_ERROR_F("Failed to parse listener address: {}", address);
				} else {
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

bool TorProcess::RemoveListener(const TorAddress& torAddress)
{
	std::unique_lock<std::mutex> lock(m_mutex);

	try
	{
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

std::shared_ptr<TorConnection> TorProcess::Connect(const TorAddress& address)
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