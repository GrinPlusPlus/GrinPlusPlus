#pragma once

#include <Net/Clients/Client.h>
#include <Net/Clients/HTTP/HTTP.h>
#include <Net/Clients/HTTP/HTTPException.h>
#include <sstream>

class IHTTPClient : public Client<HTTP::Request, HTTP::Response>
{
public:
	IHTTPClient() : m_connected(false) { }
	virtual ~IHTTPClient() = default;

	virtual HTTP::Response Invoke(const HTTP::Request& request) override final
	{
		try
		{
			if (!m_connected)
			{
				EstablishConnection(request.GetHost(), request.GetPort());
				m_connected = true;
			}

			std::string lineToWrite = request.ToString();
			Write(lineToWrite, asio::chrono::seconds(1));

			std::string responseLine = ReadLine(asio::chrono::seconds(10));
			std::istringstream responseStream(responseLine);

			std::string http_version;
			responseStream >> http_version;

			unsigned int statusCode;
			responseStream >> statusCode;

			std::string statusMessage;
			std::getline(responseStream, statusMessage);

			size_t contentLength = 0;

			std::vector<HTTP::Header> headers;
			std::string header = ReadLine(asio::chrono::seconds(1));
			while (header != "\r")
			{
				std::vector<std::string> headerParts = StringUtil::Split(header, ":");
				if (headerParts.size() < 2)
				{
					throw HTTP_EXCEPTION("Invalid header: " + header);
				}

				if (StringUtil::ToLower(StringUtil::Trim(headerParts[0])) == "content-length")
				{
					std::istringstream contentLengthStream(headerParts[1]);
					contentLengthStream >> contentLength;
				}

				headers.push_back(HTTP::Header(StringUtil::Trim(headerParts[0]), StringUtil::Trim(headerParts[1])));

				header = ReadLine(asio::chrono::seconds(1));
			}

			if (contentLength == 0)
			{
				throw HTTP_EXCEPTION("No Content-Length provided");
			}

			std::vector<uint8_t> body = Read(contentLength, asio::chrono::seconds(2));

			return HTTP::Response(statusCode, std::move(headers), std::string(body.begin(), body.end()));
		}
		catch (HTTPException&)
		{
			throw;
		}
		catch (std::exception& e)
		{
			throw HTTP_EXCEPTION(e.what());
		}
	}

private:
	virtual void EstablishConnection(const std::string& host, const uint16_t port) = 0;

	bool m_connected;
};

class HTTPClient : public IHTTPClient
{
public:
	HTTPClient() = default;
	virtual ~HTTPClient() = default;

private:
	virtual void EstablishConnection(const std::string& ipAddress, const uint16_t port) override final
	{
		Connect(SocketAddress(IPAddress::FromString(ipAddress), port), std::chrono::seconds(2));
	}
};