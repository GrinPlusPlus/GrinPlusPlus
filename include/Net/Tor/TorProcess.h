#pragma once

#include <Crypto/Models/ed25519_secret_key.h>
#include <Net/Tor/TorAddress.h>

#include <thread>
#include <mutex>

// Forward Declarations
class TorControl;
class TorConnection;

class TorProcess
{
public:
	using Ptr = std::shared_ptr<TorProcess>;

	~TorProcess();

	static TorProcess::Ptr Initialize(
		const fs::path& torDataPath,
		const uint16_t socksPort,
		const uint16_t controlPort
	) noexcept;

	std::shared_ptr<TorAddress> AddListener(const ed25519_secret_key_t& secretKey, const uint16_t portNumber);
	std::shared_ptr<TorAddress> AddListener(const std::string& serializedKey, const uint16_t portNumber);
	bool RemoveListener(const TorAddress& torAddress);

	std::shared_ptr<TorConnection> Connect(const TorAddress& address);

	// Returns true if TorControl connection is established.
	bool RetryInit();

private:
	TorProcess(const fs::path& torDataPath, const uint16_t socksPort, const uint16_t controlPort)
		: m_torDataPath(torDataPath), m_socksPort(socksPort), m_controlPort(controlPort), m_pControl(nullptr) { }

	static void Thread_Initialize(TorProcess* pProcess);
	static bool IsPortOpen(const uint16_t port);

	fs::path m_torDataPath;
	uint16_t m_socksPort;
	uint16_t m_controlPort;
	std::shared_ptr<TorControl> m_pControl;

	std::mutex m_mutex;
	std::thread m_initThread;
};