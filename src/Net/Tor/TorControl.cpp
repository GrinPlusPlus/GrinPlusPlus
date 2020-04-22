#include "TorControl.h"

#include <Net/Tor/TorException.h>
#include <filesystem.h>
#include <Crypto/Crypto.h>
#include <Net/SocketException.h>
#include <Common/Util/StringUtil.h>
#include <cppcodec/base64_rfc4648.hpp>

TorControl::TorControl(const TorConfig& config, std::shared_ptr<TorControlClient> pClient, ChildProcess::CPtr pProcess)
	: m_torConfig(config), m_pClient(pClient), m_pProcess(pProcess)
{

}

std::shared_ptr<TorControl> TorControl::Create(const TorConfig& torConfig) noexcept
{
	try
	{
		const fs::path command = fs::current_path() / "tor" / "tor";
		auto dataDirectory = fs::current_path() / "tor" / ("data" + std::to_string(torConfig.GetControlPort()));
		fs::create_directories(dataDirectory);

		std::vector<std::string> args({
			command.u8string(),
			"--ControlPort", std::to_string(torConfig.GetControlPort()),
			"--SocksPort", std::to_string(torConfig.GetSocksPort()),
			"--DataDirectory", dataDirectory.u8string(),
			"--HashedControlPassword", torConfig.GetHashedControlPassword(),
			"-f", (fs::current_path() / "tor" / ".torrc").u8string(),
			"--ignore-missing-torrc"
		});

		// TODO: Determine if process is already running.
		ChildProcess::CPtr pProcess = ChildProcess::Create(args);
		if (pProcess == nullptr)
		{
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

		if (connected && Authenticate(pClient, torConfig.GetControlPassword()))
		{
			return std::unique_ptr<TorControl>(new TorControl(torConfig, pClient, pProcess));
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception caught: {}", e.what());
	}

	return nullptr;
}

bool TorControl::Authenticate(std::shared_ptr<TorControlClient> pClient, const std::string& password)
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

std::string TorControl::AddOnion(const SecretKey64& secretKey, const uint16_t externalPort, const uint16_t internalPort)
{
	std::string serializedKey = cppcodec::base64_rfc4648::encode(secretKey.GetVec());

	return AddOnion(serializedKey, externalPort, internalPort);
}

std::string TorControl::AddOnion(const std::string& serializedKey, const uint16_t externalPort, const uint16_t internalPort)
{
	// ADD_ONION ED25519-V3:<SERIALIZED_KEY> PORT=External,Internal
	std::string command = StringUtil::Format("ADD_ONION ED25519-V3:{} Flags=DiscardPK Port={},{}\n", serializedKey, externalPort, internalPort);

	std::vector<std::string> response = m_pClient->Invoke(command);
	for (std::string& line : response)
	{
		if (StringUtil::StartsWith(line, "250-ServiceID="))
		{
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