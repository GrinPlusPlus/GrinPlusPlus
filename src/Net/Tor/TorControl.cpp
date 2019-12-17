#include "TorControl.h"

#include <Net/Tor/TorException.h>
#include <filesystem.h>
#include <Crypto/Crypto.h>
#include <Common/Base64.h>
#include <Net/SocketException.h>
#include <Common/Util/ProcessUtil.h>
#include <Common/Util/StringUtil.h>

TorControl::TorControl(const TorConfig& config, std::shared_ptr<TorControlClient> pClient, long processId)
	: m_torConfig(config), m_pClient(pClient), m_processId(processId)
{

}

TorControl::~TorControl()
{
	ProcessUtil::KillProc(m_processId);
}

std::shared_ptr<TorControl> TorControl::Create(const TorConfig& torConfig)
{
	try
	{
		#ifdef _WIN32
		const std::string command = fs::current_path().string() + "/tor/tor";
		#else
		const std::string command = "./tor/tor";
		#endif
		
		std::vector<std::string> args({
			"--ControlPort", std::to_string(torConfig.GetControlPort()),
			"--SocksPort", std::to_string(torConfig.GetSocksPort()),
			"--HashedControlPassword", torConfig.GetHashedControlPassword()
		});

		// TODO: Determine if process is already running.
		long processId = (long)ProcessUtil::CreateProc(command, args);
		if (processId > 0)
		{
			std::this_thread::sleep_for(std::chrono::seconds(2));
			// Open control socket
			std::shared_ptr<TorControlClient> pClient = std::shared_ptr<TorControlClient>(new TorControlClient());
			bool connected = pClient->Connect(SocketAddress("127.0.0.1", torConfig.GetControlPort()));
			if (!connected || !Authenticate(pClient, torConfig.GetControlPassword()))
			{
				ProcessUtil::KillProc(processId);
			}
			else
			{
				return std::unique_ptr<TorControl>(new TorControl(torConfig, pClient, processId));
			}
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
	std::string serializedKey = Base64::EncodeBase64(secretKey.GetVec());

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