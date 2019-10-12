#include "TorControl.h"
#include "TorException.h"

#include <filesystem.h>
#include <Crypto/Crypto.h>
#include <Common/Base64.h>
#include <Net/SocketException.h>
#include <Common/Util/ProcessUtil.h>
#include <Common/Util/StringUtil.h>

TorControl::TorControl(const TorConfig& config)
	: m_torConfig(config),
	m_socket(SocketAddress(IPAddress::FromString("127.0.0.1"), config.GetControlPort()))
{

}

bool TorControl::Initialize()
{
	// TODO: Determine if process is already running.

	const std::string command = StringUtil::Format(
		fs::current_path().string() + "/tor/tor --ControlPort %d --SocksPort %d --HashedControlPassword %s",
		m_torConfig.GetControlPort(),
		m_torConfig.GetSocksPort(),
		m_torConfig.GetHashedControlPassword().c_str()
	);

	m_processId = ProcessUtil::CreateProc(command);
	if (m_processId > 0)
	{
		// Open control socket
		bool connected = m_socket.Connect(m_context);
		if (!connected || !Authenticate(m_torConfig.GetControlPassword()))
		{
			m_socket.CloseSocket();
			ProcessUtil::KillProc(m_processId);
			m_processId = 0;
		}
		else
		{
			return true;
		}
	}

	return false;
}

void TorControl::Shutdown()
{
	ProcessUtil::KillProc(m_processId);
}

bool TorControl::Authenticate(const std::string& password)
{
	std::string command = StringUtil::Format("AUTHENTICATE \"%s\"", password.c_str());

	try
	{
		m_socket.SendLine(command);

		std::string lineRead;
		while (!lineRead.empty() || m_socket.ReceiveLine(lineRead))
		{
			std::vector<std::string> lines = StringUtil::Split(lineRead, "\r\n");

			const std::string line = lines[0];
			lineRead = "";
			for (size_t i = 1; i < lines.size(); i++)
			{
				lineRead += lines[i] + "\r\n";
			}

			if (!StringUtil::StartsWith(line, "250"))
			{
				return false;
			}
			else if (StringUtil::StartsWith(line, "250 OK"))
			{
				return true;
			}
		}
	}
	catch (const SocketException& e)
	{
		return false;
	}

	return false;
}

std::string TorControl::AddOnion(const SecretKey& privateKey, const uint16_t externalPort, const uint16_t internalPort)
{
	std::string address = "";

	std::string serializedKey = FormatKey(privateKey);

	// ADD_ONION ED25519-V3:<SERIALIZED_KEY> PORT=External,Internal
	std::string command = StringUtil::Format("ADD_ONION ED25519-V3:%s Flags=DiscardPK Port=%d,%d", serializedKey.c_str(), externalPort, internalPort);

	try
	{
		m_socket.SendLine(command);

		std::string lineRead;
		while (!lineRead.empty() || m_socket.ReceiveLine(lineRead))
		{
			std::vector<std::string> lines = StringUtil::Split(lineRead, "\r\n");

			const std::string line = lines[0];
			lineRead = "";
			for (size_t i = 1; i < lines.size(); i++)
			{
				lineRead += lines[i] + "\r\n";
			}

			if (!StringUtil::StartsWith(line, "250"))
			{
				throw TOR_EXCEPTION(line);
			}
			else if (StringUtil::StartsWith(line, "250-ServiceID="))
			{
				const size_t prefix = std::string("250-ServiceID=").size();
				address = line.substr(prefix);
			}
			else if (StringUtil::StartsWith(line, "250 OK"))
			{
				return address;
			}
		}
	}
	catch (const SocketException& e)
	{
		throw TOR_EXCEPTION("Socket exception occurred.");
	}

	throw TOR_EXCEPTION("OK not found.");
}

bool TorControl::DelOnion(const TorAddress& torAddress)
{
	// DEL_ONION ServiceId
	std::string command = StringUtil::Format("DEL_ONION %s", torAddress.ToString().c_str());

	try
	{
		m_socket.SendLine(command);

		std::string line;
		if (!m_socket.ReceiveLine(line))
		{
			return false;
		}

		return StringUtil::StartsWith(line, "250");
	}
	catch (const SocketException& e)
	{
		throw TOR_EXCEPTION("Socket exception occurred.");
	}
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