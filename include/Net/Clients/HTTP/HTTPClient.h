#pragma once

#include <Net/Clients/Client.h>
#include <Net/Clients/HTTP/HTTP.h>
#include <Net/Clients/HTTP/HTTPException.h>
#include <Common/Logger.h>
#include <Common/GrinStr.h>
#include <sstream>

class IHTTPClient : public Client<HTTP::Request, HTTP::Response>
{
public:
	IHTTPClient() : m_connected(false) { }
	virtual ~IHTTPClient() = default;

	HTTP::Response Invoke(const HTTP::Request& request) final
	{
		//WALLET_INFO_F("Request: {}", request.ToString());
		try
		{
			//if (!m_connected)
			{
				std::this_thread::sleep_for(std::chrono::seconds(1));
				EstablishConnection(request.GetHost(), request.GetPort());
				WALLET_INFO("Connection established");
				m_connected = true;
			}

			std::string lineToWrite = request.ToString();
			Write(lineToWrite, asio::chrono::seconds(2));

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
			GrinStr header = ReadLine(asio::chrono::seconds(1));
			while (header != "\r")
			{
				std::vector<GrinStr> headerParts = header.Split(":");
				if (headerParts.size() < 2)
				{
					throw HTTP_EXCEPTION("Invalid header: " + header);
				}

				if (headerParts[0].Trim().ToLower() == "content-length")
				{
					std::istringstream contentLengthStream(headerParts[1]);
					contentLengthStream >> contentLength;
				}

				headers.push_back(HTTP::Header{
					headerParts[0].Trim(),
					headerParts[1].Trim()
				});

				header = ReadLine(asio::chrono::seconds(1));
			}

			if (contentLength == 0)
			{
				throw HTTP_EXCEPTION("No Content-Length provided");
			}

			std::vector<uint8_t> body = Read(contentLength, asio::chrono::seconds(2));

			return HTTP::Response(
				statusCode,
				std::move(headers),
				std::string(body.begin(), body.end())
			);
		}
		catch (HTTPException& e)
		{
			WALLET_INFO_F("HTTPException: {}", e.what());
			throw e;
		}
		catch (std::exception& e)
		{
			WALLET_INFO_F("Exception: {}", e.what());
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
	void EstablishConnection(const std::string& address, const uint16_t port) final
	{
		std::error_code ec;
		auto ipAddress = asio::ip::make_address_v4(address, ec);
		if (ec)
		{
			Connect(address, port, std::chrono::seconds(5));
		}
		else
		{
			Connect(SocketAddress(IPAddress(ipAddress), port), std::chrono::seconds(5));
		}
	}
};