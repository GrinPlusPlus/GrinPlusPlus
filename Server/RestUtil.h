#pragma once

#include "civetweb/include/civetweb.h"
#include "RestException.h"

#include <Common/Util/StringUtil.h>
#include <Core/Serialization/DeserializationException.h>
#include <json/json.h>
#include <string>
#include <optional>

enum class EHTTPMethod
{
	GET,
	POST
};

class RestUtil
{
public:
	// Strips away the base URI and any query strings.
	// Ex: Given "/v1/blocks/<hash>?compact" and baseURI "/v1/blocks/", this would return <hash>.
	static std::string GetURIParam(struct mg_connection* conn, const std::string& baseURI)
	{
		const struct mg_request_info* req_info = mg_get_request_info(conn);
		const std::string requestURI(req_info->request_uri);
		
		return requestURI.substr(baseURI.size(), requestURI.size() - baseURI.size());
	}

	static std::string GetQueryString(struct mg_connection* conn)
	{
		const struct mg_request_info* req_info = mg_get_request_info(conn);
		if (req_info->query_string == nullptr)
		{
			return "";
		}

		return req_info->query_string;
	}

	static bool HasQueryParam(struct mg_connection* conn, const std::string& parameterName)
	{
		const std::string queryString = RestUtil::GetQueryString(conn);
		if (!queryString.empty())
		{
			std::vector<std::string> tokens = StringUtil::Split(queryString, "&");
			for (const std::string& token : tokens)
			{
				if (token == parameterName || StringUtil::StartsWith(token, parameterName + "="))
				{
					return true;
				}
			}
		}

		return false;
	}

	static std::optional<std::string> GetQueryParam(struct mg_connection* conn, const std::string& parameterName)
	{
		const std::string queryString = RestUtil::GetQueryString(conn);
		if (!queryString.empty())
		{
			std::vector<std::string> tokens = StringUtil::Split(queryString, "&");
			for (const std::string& token : tokens)
			{
				if (token == parameterName)
				{
					return std::make_optional<std::string>("");
				}

				if (StringUtil::StartsWith(token, parameterName + "="))
				{
					std::vector<std::string> parameterTokens = StringUtil::Split(token, "=");
					if (parameterTokens.size() != 2)
					{
						throw RestException();
					}

					return std::make_optional<std::string>(parameterTokens[1]);
				}
			}
		}

		return std::nullopt;
	}

	static std::optional<std::string> GetHeaderValue(mg_connection* conn, const std::string& headerName)
	{
		const char* pHeaderValue = mg_get_header(conn, headerName.c_str());
		if (pHeaderValue != nullptr)
		{
			return std::make_optional<std::string>(std::string(pHeaderValue));
		}

		return std::nullopt;
	}

	static EHTTPMethod GetHTTPMethod(struct mg_connection* conn)
	{
		const struct mg_request_info* req_info = mg_get_request_info(conn);
		if (req_info->request_method == std::string("GET"))
		{
			return EHTTPMethod::GET;
		}
		else if (req_info->request_method == std::string("POST"))
		{
			return EHTTPMethod::POST;
		}
		
		throw RestException();
	}

	static std::optional<Json::Value> GetRequestBody(mg_connection* conn)
	{
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
			throw RestException();
		}

		Json::Value json;
		Json::Reader reader;
		if (!reader.parse(requestBody, json))
		{
			throw DeserializationException();
		}

		return std::make_optional<Json::Value>(json);
	}

	static int BuildSuccessResponseJSON(struct mg_connection* conn, const Json::Value& json)
	{
		Json::StreamWriterBuilder builder;
		builder["indentation"] = ""; // Removes whitespaces
		const std::string output = Json::writeString(builder, json);
		return RestUtil::BuildSuccessResponse(conn, output);
	}

	static int BuildSuccessResponse(struct mg_connection* conn, const std::string& response)
	{
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