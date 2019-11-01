#include "TorControl.h"

#include <Net/Tor/TorException.h>
#include <filesystem.h>
#include <Crypto/Crypto.h>
#include <Common/Base64.h>
#include <Net/SocketException.h>
#include <Common/Util/ProcessUtil.h>
#include <Common/Util/StringUtil.h>

TorControl::TorControl(const TorConfig& config)
	: m_torConfig(config), m_initialized(false)
{

}

bool TorControl::Initialize()
{
	if (m_initialized)
	{
		return true;
	}

	try
	{
		const std::string command = StringUtil::Format(
			fs::current_path().string() + "/tor/tor --ControlPort %d --SocksPort %d --HashedControlPassword %s",
			m_torConfig.GetControlPort(),
			m_torConfig.GetSocksPort(),
			m_torConfig.GetHashedControlPassword()
		);

		// TODO: Determine if process is already running.
		m_processId = ProcessUtil::CreateProc(command);
		if (m_processId > 0)
		{
			// Open control socket
			bool connected = m_client.Connect(SocketAddress(IPAddress::FromString("127.0.0.1"), m_torConfig.GetControlPort()));
			if (!connected || !Authenticate(m_torConfig.GetControlPassword()))
			{
				ProcessUtil::KillProc(m_processId);
				m_processId = 0;
			}
			else
			{
				m_initialized = true;
				return true;
			}
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception caught: %s", e.what());
	}

	return false;
}

void TorControl::Shutdown()
{
	ProcessUtil::KillProc(m_processId);
}

bool TorControl::Authenticate(const std::string& password)
{
	std::string command = StringUtil::Format("AUTHENTICATE \"%s\"\n", password);

	try
	{
		m_client.Invoke(command);
		return true;
	}
	catch (TorException& e)
	{
		return false;
	}
}

std::string TorControl::AddOnion(const SecretKey& privateKey, const uint16_t externalPort, const uint16_t internalPort)
{
	std::string address = "";

	std::string serializedKey = FormatKey(privateKey);

	// ADD_ONION ED25519-V3:<SERIALIZED_KEY> PORT=External,Internal
	std::string command = StringUtil::Format("ADD_ONION ED25519-V3:%s Flags=DiscardPK Port=%d,%d\n", serializedKey, externalPort, internalPort);

	std::vector<std::string> response = m_client.Invoke(command);
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

	m_client.Invoke(command);
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