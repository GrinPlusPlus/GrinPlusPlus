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
		const std::string command = StringUtil::Format(
			fs::current_path().string() + "/tor/tor --ControlPort %d --SocksPort %d --HashedControlPassword %s",
			torConfig.GetControlPort(),
			torConfig.GetSocksPort(),
			torConfig.GetHashedControlPassword()
		);

		// TODO: Determine if process is already running.
		long processId = ProcessUtil::CreateProc(command);
		if (processId > 0)
		{
			// Open control socket
			std::shared_ptr<TorControlClient> pClient = std::shared_ptr<TorControlClient>(new TorControlClient());
			bool connected = pClient->Connect(SocketAddress(IPAddress::FromString("127.0.0.1"), torConfig.GetControlPort()));
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
		LOG_ERROR_F("Exception caught: %s", e.what());
	}

	return nullptr;
}

bool TorControl::Authenticate(std::shared_ptr<TorControlClient> pClient, const std::string& password)
{
	std::string command = StringUtil::Format("AUTHENTICATE \"%s\"\n", password);

	try
	{
		pClient->Invoke(command);
		return true;
	}
	catch (TorException& e)
	{
		return false;
	}
}

std::string TorControl::AddOnion(const SecretKey& privateKey, const uint16_t externalPort, const uint16_t internalPort)
{
	std::string serializedKey = FormatKey(privateKey);

	// ADD_ONION ED25519-V3:<SERIALIZED_KEY> PORT=External,Internal
	std::string command = StringUtil::Format("ADD_ONION ED25519-V3:%s Flags=DiscardPK Port=%d,%d\n", serializedKey, externalPort, internalPort);

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
	std::string command = StringUtil::Format("DEL_ONION %s\n", torAddress.ToString());

	m_pClient->Invoke(command);
	return true;
}

std::string TorControl::FormatKey(const SecretKey& privateKey) const
{
	/*
		For a "ED25519-V3" key is
		the Base64 encoding of the concatenation of the 32-byte ed25519 secret
		scalar in little-endian and the 32-byte ed25519 PRF secret.)

		[Note: The ED25519-V3 format is not the same as, e.g., SUPERCOP
		ed25519/ref, which stores the concatenation of the 32-byte ed25519
		hash seed concatenated with the 32-byte public key, and which derives
		the secret scalar and PRF secret by expanding the hash seed with
		SHA-512.  Our key blinding scheme is incompatible with storing
		private keys as seeds, so we store the secret scalar alongside the
		PRF secret, and just pay the cost of recomputing the public key when
		importing an ED25519-V3 key.]
	*/
	CBigInteger<64> hash = Crypto::SHA512(privateKey.GetBytes().GetData());
	hash[0] &= 248;
	hash[31] &= 127;
	hash[31] |= 64;

	return Base64::EncodeBase64(hash.GetData());
}