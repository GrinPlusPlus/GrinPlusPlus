#include "TorControl.h"
#include "TorControlClient.h"

#include <Net/Tor/TorException.h>
#include <Common/Util/StringUtil.h>
#include <cppcodec/base64_rfc4648.hpp>
#include <Crypto/ED25519.h>
#include <filesystem.h>

TorControl::TorControl(const TorConfig& config, std::shared_ptr<TorControlClient> pClient, ChildProcess::UCPtr&& pProcess)
	: m_torConfig(config), m_pClient(pClient), m_pProcess(std::move(pProcess))
{

}

std::shared_ptr<TorControl> TorControl::Create(const TorConfig& torConfig) noexcept
{
	try
	{
		const fs::path command = fs::current_path() / "tor" / "tor";

#ifdef __linux__
		std::error_code ec;
		fs::path torDataPath = torConfig.GetTorDataPath() / ("data" + std::to_string(torConfig.GetControlPort()));
		fs::remove_all(torDataPath, ec);
		fs::create_directories(torDataPath, ec);
		std::string torrcPath = (torConfig.GetTorDataPath() / ".torrc").u8string();
#else
		std::error_code ec;
		fs::path torDataPath = "./tor/data" + std::to_string(torConfig.GetControlPort());
		fs::remove_all(torDataPath, ec);
		fs::create_directories(torDataPath, ec);
		fs::remove("./tor/.torrc", ec);
		fs::copy_file(torConfig.GetTorDataPath() / ".torrc", "./tor/.torrc", ec);
		std::string torrcPath = "./tor/.torrc";
#endif

		std::vector<std::string> args({
			command.u8string(),
			"--ControlPort", std::to_string(torConfig.GetControlPort()),
			"--SocksPort", std::to_string(torConfig.GetSocksPort()),
			"--DataDirectory", torDataPath.u8string(),
			"--HashedControlPassword", torConfig.GetHashedControlPassword(),
			"-f", torrcPath,
			"--ignore-missing-torrc"
		});

		// TODO: Determine if process is already running.
		ChildProcess::UCPtr pProcess = ChildProcess::Create(args);
		if (pProcess == nullptr) {
			// Fallback to tor on path
			args[0] = "tor";
			pProcess = ChildProcess::Create(args);
		}

		std::shared_ptr<TorControlClient> pClient = std::shared_ptr<TorControlClient>(new TorControlClient());

		bool connected = false;
		auto timeout = std::chrono::system_clock::now() + std::chrono::seconds(10);
		while (!connected && std::chrono::system_clock::now() < timeout)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));

			// Open control socket
			connected = pClient->Connect(SocketAddress("127.0.0.1", torConfig.GetControlPort()));
		}

		if (connected && Authenticate(pClient, torConfig.GetControlPassword())) {
			return std::unique_ptr<TorControl>(new TorControl(torConfig, pClient, std::move(pProcess)));
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception caught: {}", e.what());
	}

	return nullptr;
}

bool TorControl::Authenticate(const std::shared_ptr<TorControlClient>& pClient, const std::string& password)
{
	std::string command = StringUtil::Format("AUTHENTICATE \"{}\"\n", password);

	try
	{
		pClient->Invoke(command);
		return true;
	}
	catch (TorException&)
	{
		return false;
	}
}

std::string TorControl::AddOnion(const ed25519_secret_key_t& secretKey, const uint16_t externalPort, const uint16_t internalPort)
{
	// "ED25519-V3" key is the Base64 encoding of the concatenation of
	// the 32-byte ed25519 secret scalar in little-endian
	// and the 32-byte ed25519 PRF secret.
	std::string serializedKey = cppcodec::base64_rfc4648::encode(ED25519::CalculateTorKey(secretKey).GetVec());

	return AddOnion(serializedKey, externalPort, internalPort);
}

std::string TorControl::AddOnion(const std::string& serializedKey, const uint16_t externalPort, const uint16_t internalPort)
{
	// ADD_ONION ED25519-V3:<SERIALIZED_KEY> PORT=External,Internal
	std::string command = StringUtil::Format("ADD_ONION ED25519-V3:{} Flags=DiscardPK Port={},{}\n", serializedKey, externalPort, internalPort);

	std::vector<std::string> response = m_pClient->Invoke(command);
	for (std::string& line : response)
	{
		if (StringUtil::StartsWith(line, "250-ServiceID=")) {
			const size_t prefix = std::string("250-ServiceID=").size();
			return line.substr(prefix);
		}
	}

	throw TOR_EXCEPTION("Address not returned");
}

bool TorControl::DelOnion(const TorAddress& torAddress)
{
	// DEL_ONION ServiceId
	std::string command = StringUtil::Format("DEL_ONION {}\n", torAddress.ToString());

	m_pClient->Invoke(command);
	return true;
}