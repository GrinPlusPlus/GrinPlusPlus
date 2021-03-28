#pragma once

#include <Crypto/Models/ed25519_secret_key.h>
#include <Net/Tor/TorAddress.h>
#include <Common/ConcurrentQueue.h>

#include <atomic>
#include <thread>
#include <mutex>
#include <map>

// Forward Declarations
class TorControl;
class ITorConnection;

class ITorProcess
{
public:
	using Ptr = std::shared_ptr<ITorProcess>;

	virtual ~ITorProcess() = default;

	virtual TorAddress AddListener(const ed25519_secret_key_t& secretKey, const uint16_t portNumber) = 0;
	virtual bool RemoveListener(const TorAddress& torAddress) = 0;

	virtual std::shared_ptr<ITorConnection> Connect(const TorAddress& address) = 0;

	virtual bool RetryInit() = 0;
};

class TorProcess : public ITorProcess
{
public:
	using Ptr = std::shared_ptr<ITorProcess>;

	virtual ~TorProcess();

	static TorProcess::Ptr Initialize(
		const fs::path& torDataPath,
		const uint16_t socksPort,
		const uint16_t controlPort
	) noexcept;

	TorAddress AddListener(const ed25519_secret_key_t& secretKey, const uint16_t portNumber) final;
	bool RemoveListener(const TorAddress& torAddress) final;

	std::shared_ptr<ITorConnection> Connect(const TorAddress& address) final;

	bool RetryInit() final;

private:
	TorProcess(const fs::path& torDataPath, const uint16_t socksPort, const uint16_t controlPort)
		: m_torDataPath(torDataPath), m_socksPort(socksPort), m_controlPort(controlPort), m_pControl(nullptr), m_shutdown(false) { }

	static void Thread_Initialize(TorProcess* pProcess);
	static bool IsPortOpen(const uint16_t port);

	std::unique_ptr<TorAddress> AddListener(
		const std::unique_lock<std::mutex>&,
		const ed25519_secret_key_t& secretKey,
		const uint16_t portNumber
	);

	fs::path m_torDataPath;
	uint16_t m_socksPort;
	uint16_t m_controlPort;
	std::shared_ptr<TorControl> m_pControl;
	std::map<std::string, std::pair<ed25519_secret_key_t, uint16_t>> m_activeServices;

	ConcurrentQueue<std::pair<ed25519_secret_key_t, uint16_t>> m_servicesToAdd;

	std::atomic_bool m_shutdown;
	std::mutex m_mutex;
	std::thread m_initThread;
};