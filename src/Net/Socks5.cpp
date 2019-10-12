#include <Net/Socks5.h>
#include <Net/SocketException.h>
#include <Common/Util/TimeUtil.h>

#include <inttypes.h>
#include <vector>
#include <algorithm>
#include <atomic>


#if !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0
#endif

static const int SOCKS5_RECV_TIMEOUT = 20 * 1000;
static std::atomic<bool> interruptSocks5Recv(false);

/** SOCKS version */
enum SOCKSVersion : uint8_t
{
	SOCKS4 = 0x04,
	SOCKS5 = 0x05
};

/** Values defined for METHOD in RFC1928 */
enum SOCKS5Method : uint8_t
{
	NOAUTH = 0x00,        //! No authentication required
	GSSAPI = 0x01,        //! GSSAPI
	USER_PASS = 0x02,     //! Username/password
	NO_ACCEPTABLE = 0xff, //! No acceptable methods
};

/** Values defined for CMD in RFC1928 */
enum SOCKS5Command : uint8_t
{
	CONNECT = 0x01,
	BIND = 0x02,
	UDP_ASSOCIATE = 0x03
};

/** Values defined for REP in RFC1928 */
enum SOCKS5Reply : uint8_t
{
	SUCCEEDED = 0x00,        //! Succeeded
	GENFAILURE = 0x01,       //! General failure
	NOTALLOWED = 0x02,       //! Connection not allowed by ruleset
	NETUNREACHABLE = 0x03,   //! Network unreachable
	HOSTUNREACHABLE = 0x04,  //! Network unreachable
	CONNREFUSED = 0x05,      //! Connection refused
	TTLEXPIRED = 0x06,       //! TTL expired
	CMDUNSUPPORTED = 0x07,   //! Command not supported
	ATYPEUNSUPPORTED = 0x08, //! Address type not supported
};

/** Values defined for ATYPE in RFC1928 */
enum SOCKS5Atyp : uint8_t
{
	IPV4 = 0x01,
	DOMAINNAME = 0x03,
	IPV6 = 0x04,
};

/** Status codes that can be returned by InterruptibleRecv */
enum class IntrRecvError
{
	OK,
	Timeout,
	Disconnected,
	NetworkError,
	Interrupted
};

int64_t GetTimeMillis()
{
	return (int64_t)TimeUtil::Now();
}

struct timeval MillisToTimeval(int64_t nTimeout)
{
	struct timeval timeout;
	timeout.tv_sec = nTimeout / 1000;
	timeout.tv_usec = (nTimeout % 1000) * 1000;
	return timeout;
}

/**
 * Read bytes from socket. This will either read the full number of bytes requested
 * or return False on error or timeout.
 * This function can be interrupted by calling InterruptSocks5()
 *
 * @param data Buffer to receive into
 * @param len  Length of data to receive
 * @param timeout  Timeout in milliseconds for receive operation
 *
 * @note This function requires that hSocket is in non-blocking mode.
 */
static IntrRecvError InterruptibleRecv(uint8_t* data, size_t len, int timeout, Socket& socket)
{

	int64_t curTime = GetTimeMillis();
	int64_t endTime = curTime + timeout;
	// Maximum time to wait in one select call. It will take up until this time (in millis)
	// to break off in case of an interruption.
	const int64_t maxWait = 1000;
	while (curTime < endTime)
	{
		try
		{
			std::vector<unsigned char> bytes(len);
			if (socket.Receive(len, false, bytes))
			{
				memcpy_s(data, len, bytes.data(), bytes.size());
				return IntrRecvError::OK;
			}
		}
		catch (const SocketException& e)
		{
			return IntrRecvError::Disconnected;
		}

		curTime = GetTimeMillis();
	}

	return IntrRecvError::Timeout;
}

/** Credentials for proxy authentication */
struct ProxyCredentials
{
	std::string username;
	std::string password;
};

/** Convert SOCKS5 reply to an error message */
static std::string Socks5ErrorString(uint8_t err)
{
	switch (err)
	{
		case SOCKS5Reply::GENFAILURE:
			return "general failure";
		case SOCKS5Reply::NOTALLOWED:
			return "connection not allowed";
		case SOCKS5Reply::NETUNREACHABLE:
			return "network unreachable";
		case SOCKS5Reply::HOSTUNREACHABLE:
			return "host unreachable";
		case SOCKS5Reply::CONNREFUSED:
			return "connection refused";
		case SOCKS5Reply::TTLEXPIRED:
			return "TTL expired";
		case SOCKS5Reply::CMDUNSUPPORTED:
			return "protocol error";
		case SOCKS5Reply::ATYPEUNSUPPORTED:
			return "address type not supported";
		default:
			return "unknown";
	}
}

bool Socks5Connection::Connect(const SocketAddress& socksAddress, const std::string& destination, const int port)
{
	m_pSocket = std::make_unique<Socket>(Socket(socksAddress));
	if (!m_pSocket->Connect(m_context))
	{
		return false;
	}

	// LogPrint(BCLog::NET, "SOCKS5 connecting %s\n", strDest);

	if (destination.size() > 255)
	{
		return false;// error("Hostname too long");
	}

	if (!Initialize(nullptr))
	{
		return false;// error("Error sending to proxy");
	}

	if (!Authenticate(nullptr))
	{
		return false;
	}

	if (!Connect(destination, port))
	{
		return false;//error("Error sending to proxy");
	}

	// LogPrint(BCLog::NET, "SOCKS5 connected %s\n", strDest);
	return true;
}

bool Socks5Connection::Initialize(const ProxyCredentials* auth)
{
	// Accepted authentication methods
	std::vector<uint8_t> data;
	if (auth != nullptr)
	{
		data = std::vector<uint8_t>({ SOCKSVersion::SOCKS5, 0x02, SOCKS5Method::NOAUTH, SOCKS5Method::USER_PASS });
	}
	else
	{
		data = std::vector<uint8_t>({ SOCKSVersion::SOCKS5, 0x01, SOCKS5Method::NOAUTH });
	}

	return m_pSocket->Send(data, false);
}

bool Socks5Connection::Authenticate(const ProxyCredentials* pAuth)
{
	std::vector<uint8_t> received(2);
	if (!m_pSocket->Receive(2, false, received))
	{
		// LogPrintf("Socks5() connect to %s:%d failed: InterruptibleRecv() timeout or other failure\n", strDest, port);
		return false;
	}

	if (received[0] != SOCKSVersion::SOCKS5)
	{
		return false;//error("Proxy failed to initialize");
	}

	if (received[1] == SOCKS5Method::USER_PASS && pAuth != nullptr)
	{
		if (pAuth->username.size() > 255 || pAuth->password.size() > 255)
		{
			return false;// error("Proxy username or password too long");
		}

		// Perform username/password authentication (as described in RFC1929)
		std::vector<uint8_t> data;
		data.push_back(0x01); // Current (and only) version of user/pass subnegotiation
		data.push_back(pAuth->username.size());
		data.insert(data.end(), pAuth->username.begin(), pAuth->username.end());
		data.push_back(pAuth->password.size());
		data.insert(data.end(), pAuth->password.begin(), pAuth->password.end());

		if (!m_pSocket->Send(data, false))
		{
			return false;//error("Error sending authentication to proxy");
		}

		// LogPrint(BCLog::PROXY, "SOCKS5 sending proxy authentication %s:%s\n", auth->username, auth->password);

		std::vector<uint8_t> authResponse(2);
		if (!m_pSocket->Receive(2, false, authResponse))
		{
			return false;//error("Error reading proxy authentication response");
		}

		if (authResponse[0] != 0x01 || authResponse[1] != 0x00)
		{
			return false;//error("Proxy authentication unsuccessful");
		}
	}
	else if (received[1] == SOCKS5Method::NOAUTH)
	{
		// Perform no authentication
	}
	else
	{
		return false;//error("Proxy requested wrong authentication method %02x", pchRet1[1]);
	}

	return true;
}

bool Socks5Connection::Connect(const std::string& destination, const int port)
{
	std::vector<uint8_t> data;
	data.push_back(SOCKSVersion::SOCKS5); // VER protocol version
	data.push_back(SOCKS5Command::CONNECT); // CMD CONNECT
	data.push_back(0x00); // RSV Reserved must be 0
	data.push_back(SOCKS5Atyp::DOMAINNAME); // ATYP DOMAINNAME
	data.push_back(destination.size()); // Length<=255 is checked at beginning of function
	data.insert(data.end(), destination.begin(), destination.end());
	data.push_back((port >> 8) & 0xFF);
	data.push_back((port >> 0) & 0xFF);

	if (!m_pSocket->Send(data, false))
	{
		return false;
	}

	std::vector<uint8_t> connectResponse(4);
	if (!m_pSocket->Receive(4, false, connectResponse))
	{
		//if (error == IntrRecvError::Timeout)
		//{
		//	/* If a timeout happens here, this effectively means we timed out while connecting
		//	 * to the remote node. This is very common for Tor, so do not print an
		//	 * error message. */
		//	return false;
		//}
		//else
		//{
		//	return false;//error("Error while reading proxy response");
		//}
		return false;
	}

	if (connectResponse[0] != SOCKSVersion::SOCKS5)
	{
		return false;//error("Proxy failed to accept request");
	}

	if (connectResponse[1] != SOCKS5Reply::SUCCEEDED)
	{
		// Failures to connect to a peer that are not proxy errors
		// LogPrintf("Socks5() connect to %s:%d failed: %s\n", strDest, port, Socks5ErrorString(pchRet2[1]));
		return false;
	}

	if (connectResponse[2] != 0x00) // Reserved field must be 0
	{
		return false;//error("Error: malformed proxy response");
	}

	IntrRecvError error = IntrRecvError::OK;
	uint8_t pchRet3[256];
	switch (connectResponse[3])
	{
		case SOCKS5Atyp::IPV4:
		{
			error = InterruptibleRecv(pchRet3, 4, SOCKS5_RECV_TIMEOUT, *m_pSocket);
			break;
		}
		case SOCKS5Atyp::IPV6:
		{
			error = InterruptibleRecv(pchRet3, 16, SOCKS5_RECV_TIMEOUT, *m_pSocket);
			break;
		}
		case SOCKS5Atyp::DOMAINNAME:
		{
			error = InterruptibleRecv(pchRet3, 1, SOCKS5_RECV_TIMEOUT, *m_pSocket);
			if (error != IntrRecvError::OK)
			{
				return false;//error("Error reading from proxy");
			}
			int nRecv = pchRet3[0];
			error = InterruptibleRecv(pchRet3, nRecv, SOCKS5_RECV_TIMEOUT, *m_pSocket);
			break;
		}
		default:
		{
			return false;//error("Error: malformed proxy response");
		}
	}

	if (error != IntrRecvError::OK)
	{
		return false;//error("Error reading from proxy");
	}

	if ((error = InterruptibleRecv(pchRet3, 2, SOCKS5_RECV_TIMEOUT, *m_pSocket)) != IntrRecvError::OK)
	{
		return false;//error("Error reading from proxy");
	}

	return true;
}