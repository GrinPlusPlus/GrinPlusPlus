#pragma once

#include <civetweb.h>
#include <Net/Clients/HTTP/HTTP.h>
#include <Net/Clients/HTTP/HTTPException.h>
#include <Common/Util/StringUtil.h>
#include <Core/Exceptions/DeserializationException.h>
#include <Core/Util/JsonUtil.h>
#include <json/json.h>
#include <cassert>
#include <string>
#include <optional>

class HTTPUtil
{
public:
	// Strips away the base URI and any query strings.
	// Ex: Given "/v1/blocks/<hash>?compact" and baseURI "/v1/blocks/", this would return <hash>.
	static std::string GetURIParam(struct mg_connection* conn, const std::string& baseURI)
	{
		assert(conn != nullptr);

		const struct mg_request_info* req_info = mg_get_request_info(conn);
		const std::string requestURI(req_info->request_uri);
		
		return requestURI.substr(baseURI.size(), requestURI.size() - baseURI.size());
	}

	static std::string GetQueryString(struct mg_connection* conn) noexcept
	{
		const struct mg_request_info* req_info = mg_get_request_info(conn);
		if (req_info == nullptr || req_info->query_string == nullptr)
		{
			return "";
		}

		return req_info->query_string;
	}

	static bool HasQueryParam(struct mg_connection* conn, const std::string& parameterName)
	{
		assert(conn != nullptr);

		return GetQueryParam(conn, parameterName).has_value();
	}

	static std::optional<std::string> GetQueryParam(struct mg_connection* conn, const std::string& parameterName)
	{
		assert(conn != nullptr);

		const std::string queryString = HTTPUtil::GetQueryString(conn);
		if (!queryString.empty())
		{
			std::vector<std::string> tokens = StringUtil::Split(queryString, "&");
			for (const std::string& token : tokens)
			{
				if (token == parameterName)
				{
					return std::make_optional("");
				}

				if (StringUtil::StartsWith(token, parameterName + "="))
				{
					std::vector<std::string> parameterTokens = StringUtil::Split(token, "=");
					if (parameterTokens.size() != 2)
					{
						throw HTTPException();
					}

					return std::make_optional(parameterTokens[1]);
				}
			}
		}

		return std::nullopt;
	}

	static std::optional<std::string> GetHeaderValue(mg_connection* conn, const std::string& headerName)
	{
		assert(conn != nullptr);

		const char* pHeaderValue = mg_get_header(conn, headerName.c_str());
		if (pHeaderValue != nullptr)
		{
			return std::make_optional(std::string(pHeaderValue));
		}

		return std::nullopt;
	}

	static HTTP::EHTTPMethod GetHTTPMethod(struct mg_connection* conn)
	{
		assert(conn != nullptr);

		const struct mg_request_info* req_info = mg_get_request_info(conn);
		if (req_info->request_method == std::string("GET"))
		{
			return HTTP::EHTTPMethod::GET;
		}
		else if (req_info->request_method == std::string("POST"))
		{
			return HTTP::EHTTPMethod::POST;
		}
		
		throw HTTPException();
	}

	static std::optional<Json::Value> GetRequestBody(mg_connection* conn)
	{
		assert(conn != nullptr);

		const struct mg_request_info* req_info = mg_get_request_info(conn);
		const long long contentLength = req_info->content_length;
		if (contentLength <= 0)
		{
			return std::nullopt;
		}

		std::string requestBody;
		requestBody.resize(contentLength);

		const int bytesRead = mg_read(conn, requestBody.data(), contentLength);
		if (bytesRead != contentLength)
		{
			throw HTTPException();
		}

		Json::Value json;
		if (!JsonUtil::Parse(requestBody, json))
		{
			throw DESERIALIZATION_EXCEPTION();
		}

		return std::make_optional(json);
	}

	static int BuildSuccessResponseJSON(struct mg_connection* conn, const Json::Value& json)
	{
		assert(conn != nullptr);

		Json::StreamWriterBuilder builder;
		builder["indentation"] = ""; // Removes whitespaces
		const std::string output = Json::writeString(builder, json);
		return HTTPUtil::BuildSuccessResponse(conn, output);
	}

	static int BuildSuccessResponse(struct mg_connection* conn, const std::string& response)
	{
		assert(conn != nullptr);

		unsigned long len = (unsigned long)response.size();

		if (response.empty())
		{
			mg_printf(conn,
				"HTTP/1.1 200 OK\r\n"
				"Content-Length: 0\r\n"
				"Connection: close\r\n\r\n");
		}
		else
		{
			mg_printf(conn,
				"HTTP/1.1 200 OK\r\n"
				"Content-Length: %lu\r\n"
				"Content-Type: application/json\r\n"
				"Connection: close\r\n\r\n",
				len);
		}

		mg_write(conn, response.c_str(), len);

		return 200;
	}

	static int BuildBadRequestResponse(struct mg_connection* conn, const std::string& response)
	{
		assert(conn != nullptr);

		unsigned long len = (unsigned long)response.size();

		mg_printf(conn,
			"HTTP/1.1 400 Bad Request\r\n"
			"Content-Length: %lu\r\n"
			"Content-Type: text/plain\r\n"
			"Connection: close\r\n\r\n",
			len);

		mg_write(conn, response.c_str(), len);

		return 400;
	}

	static int BuildConflictResponse(struct mg_connection* conn, const std::string& response)
	{
		assert(conn != nullptr);

		unsigned long len = (unsigned long)response.size();

		mg_printf(conn,
			"HTTP/1.1 409 Conflict\r\n"
			"Content-Length: %lu\r\n"
			"Content-Type: text/plain\r\n"
			"Connection: close\r\n\r\n",
			len);

		mg_write(conn, response.c_str(), len);

		return 409;
	}

	static int BuildUnauthorizedResponse(struct mg_connection* conn, const std::string& response)
	{
		assert(conn != nullptr);

		unsigned long len = (unsigned long)response.size();

		mg_printf(conn,
			"HTTP/1.1 401 Unauthorized\r\n"
			"Content-Length: %lu\r\n"
			"Content-Type: text/plain\r\n"
			"Connection: close\r\n\r\n",
			len);

		mg_write(conn, response.c_str(), len);

		return 401;
	}

	static int BuildNotFoundResponse(struct mg_connection* conn, const std::string& response)
	{
		assert(conn != nullptr);

		unsigned long len = (unsigned long)response.size();

		mg_printf(conn,
			"HTTP/1.1 404 Not Found\r\n"
			"Content-Length: %lu\r\n"
			"Content-Type: text/plain\r\n"
			"Connection: close\r\n\r\n",
			len);

		mg_write(conn, response.c_str(), len);

		return 400;
	}

	static int BuildInternalErrorResponse(struct mg_connection* conn, const std::string& response)
	{
		assert(conn != nullptr);

		unsigned long len = (unsigned long)response.size();

		mg_printf(conn,
			"HTTP/1.1 500 Internal Server Error\r\n"
			"Content-Length: %lu\r\n"
			"Content-Type: text/plain\r\n"
			"Connection: close\r\n\r\n",
			len);

		mg_write(conn, response.c_str(), len);

		return 500;
	}
};