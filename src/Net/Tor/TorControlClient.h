#pragma once

#include <Net/Clients/Client.h>
#include <Net/Tor/TorException.h>
#include <Infrastructure/Logger.h>

static const std::chrono::seconds TOR_CONTROL_TIMEOUT = std::chrono::seconds(3);

class TorControlClient : public Client<std::string, std::vector<std::string>>
{
public:
	bool Connect(const SocketAddress& address)
	{
		try
		{
			Client::Connect(address, TOR_CONTROL_TIMEOUT);
			return true;
		}
		catch (asio::system_error& e)
		{
			LOG_ERROR_F("Connection to %s failed with error %s.", address, e.what());
			return false;
		}
	}

	//
	// Writes the given string to the socket, and reads each line until "250 OK" is read.
	// throws TorException - If a line is read indicating a failure (ie not prefixed with "250").
	//
	virtual std::vector<std::string> Invoke(const std::string& request) override final
	{
		try
		{
			Write(request, TOR_CONTROL_TIMEOUT);

			std::vector<std::string> response;

			std::string line = StringUtil::Trim(ReadLine(TOR_CONTROL_TIMEOUT));
			while (line != "250 OK")
			{
				if (!StringUtil::StartsWith(line, "250"))
				{
					throw TOR_EXCEPTION("Failed with error: " + line);
				}

				response.push_back(line);
				line = StringUtil::Trim(ReadLine(TOR_CONTROL_TIMEOUT));
			}

			return response;
		}
		catch (TorException& e)
		{
			throw;
		}
		catch (std::exception& e)
		{
			throw TOR_EXCEPTION(e.what());
		}
	}
};