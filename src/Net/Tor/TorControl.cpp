#include "TorControl.h"
#include "TorControlClient.h"

#include <Core/Global.h>
#include <Core/Config.h>
#include <Net/Tor/TorException.h>
#include <Net/Tor/TorAddressParser.h>
#include <Common/Util/StringUtil.h>
#include <cppcodec/base64_rfc4648.hpp>
#include <Crypto/ED25519.h>
#include <filesystem.h>

TorControl::TorControl(
	const uint16_t socksPort,
	const uint16_t controlPort,
	const fs::path& torDataPath,
	std::unique_ptr<TorControlClient>&& pClient,
	ChildProcess::UCPtr&& pProcess)
	: m_socksPort(socksPort),
	m_controlPort(controlPort),
	m_torDataPath(torDataPath),
	m_pClient(std::move(pClient)),
	m_pProcess(std::move(pProcess))
{

}

TorControl::~TorControl()
{
	m_pClient.reset();
}

std::shared_ptr<TorControl> TorControl::Create(
	const uint16_t socksPort,
	const uint16_t controlPort,
	const fs::path& torDataPath) noexcept
{
	try
	{
		const fs::path command = fs::current_path() / "tor" / "tor";

#ifdef __linux__
		std::error_code ec;
		fs::path torDataDir = torDataPath / ("data" + std::to_string(controlPort));
		fs::remove_all(torDataDir, ec);
		fs::create_directories(torDataDir, ec);
		std::string torrcPath = (torDataPath / ".torrc").u8string();
#else
		std::error_code ec;
		fs::path torDataDir = "./tor/data" + std::to_string(controlPort);
		fs::remove_all(torDataDir, ec);
		fs::create_directories(torDataDir, ec);
		fs::remove("./tor/.torrc", ec);
		fs::copy_file(torDataPath / ".torrc", "./tor/.torrc", ec);
		std::string torrcPath = "./tor/.torrc";
#endif

		/*std::vector<std::string> args({
			command.u8string(),
			"--ControlPort", std::to_string(controlPort),
			"--SocksPort", std::to_string(socksPort),
			"--DataDirectory", torDataDir.u8string(),
			"--HashedControlPassword", Global::GetConfig().GetHashedControlPassword(),
			"-f", torrcPath,
			"--ignore-missing-torrc"
		});
		
		ChildProcess::UCPtr pProcess = ChildProcess::Create(args);
		if (pProcess == nullptr) {
			// Fallback to tor on path
			args[0] = "tor";
			pProcess = ChildProcess::Create(args);
		}*/

		ChildProcess::UCPtr pProcess =  nullptr;
		
		auto pClient = TorControlClient::Connect(controlPort, Global::GetConfig().GetControlPassword());
		
		if (pClient != nullptr) {
			return std::make_unique<TorControl>(
				socksPort,
				controlPort,
				torDataPath,
				std::move(pClient),
				std::move(pProcess)
			);
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception caught: {}", e.what());
	}

	return nullptr;
}

std::string TorControl::AddOnion(const ed25519_secret_key_t& secretKey, const uint16_t externalPort, const uint16_t internalPort)
{
	TorAddress tor_address = TorAddressParser::FromPubKey(ED25519::CalculatePubKey(secretKey));
	std::vector<std::string> hidden_services = QueryHiddenServices();
	for (const std::string& service : hidden_services)
	{
		if (service == tor_address.ToString()) {
			LOG_INFO_F("Hidden service already running for onion address {}", tor_address.ToString());
			return service;
		}
	}

	// "ED25519-V3" key is the Base64 encoding of the concatenation of
	// the 32-byte ed25519 secret scalar in little-endian
	// and the 32-byte ed25519 PRF secret.
	std::string serializedKey = cppcodec::base64_rfc4648::encode(ED25519::CalculateTorKey(secretKey).GetVec());

	// ADD_ONION ED25519-V3:<SERIALIZED_KEY> PORT=External,Internal
	std::string command = StringUtil::Format(
		"ADD_ONION ED25519-V3:{} Flags=DiscardPK,Detach Port={},{}\n",
		serializedKey,
		externalPort,
		internalPort
	);

	std::vector<std::string> response = m_pClient->Invoke(command);
	for (GrinStr line : response)
	{
		if (line.StartsWith("250-ServiceID=")) {
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

bool TorControl::CheckHeartbeat()
{
	try
	{
		m_pClient->Invoke("SIGNAL HEARTBEAT\n");
	}
	catch (std::exception&)
	{
		return false;
	}

	return true;
}

std::vector<std::string> TorControl::QueryHiddenServices()
{
	// GETINFO onions/detached
	std::string command = "GETINFO onions/detached\n";

	std::vector<std::string> response = m_pClient->Invoke(command);
	
	std::vector<std::string> addresses;
	bool listing_addresses = false;
	for (GrinStr line : response)
	{
		if (line.Trim() == "250 OK" || line.Trim() == ".") {
			break;
		} else if (line.StartsWith("250+onions/detached=")) {
			listing_addresses = true;

			if (line.Trim() != "250+onions/detached=") {
				auto parts = line.Split("250+onions/detached=");
				if (parts.size() == 2) {
					addresses.push_back(parts[1].Trim());
				}
			}
		} else if (listing_addresses) {
			addresses.push_back(line.Trim());
		}
	}

	return addresses;
}