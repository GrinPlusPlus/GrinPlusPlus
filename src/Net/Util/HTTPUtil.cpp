#include <Net/Util/HTTPUtil.h>
#include <Core/Util/JsonUtil.h>
#include <Net/Clients/HTTP/HTTPException.h>
#include <Common/Util/StringUtil.h>
#include <Core/Exceptions/DeserializationException.h>
#include <Common/Compat.h>

#include <civetweb.h>

// Strips away the base URI and any query strings.
// Ex: Given "/v1/blocks/<hash>?compact" and baseURI "/v1/blocks/", this would return <hash>.
std::string HTTPUtil::GetURIParam(mg_connection* conn, const std::string& baseURI)
{
	assert(conn != nullptr);

	const struct mg_request_info* req_info = mg_get_request_info(conn);
	const std::string requestURI(req_info->request_uri);

	return requestURI.substr(baseURI.size(), requestURI.size() - baseURI.size());
}

std::string HTTPUtil::GetQueryString(mg_connection* conn) noexcept
{
	const struct mg_request_info* req_info = mg_get_request_info(conn);
	if (req_info == nullptr || req_info->query_string == nullptr)
	{
		return "";
	}

	return req_info->query_string;
}

bool HTTPUtil::HasQueryParam(mg_connection* conn, const std::string& parameterName)
{
	assert(conn != nullptr);

	return GetQueryParam(conn, parameterName).has_value();
}

std::optional<std::string> HTTPUtil::GetQueryParam(mg_connection* conn, const std::string& parameterName)
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

std::optional<std::string> HTTPUtil::GetHeaderValue(mg_connection* conn, const std::string& headerName)
{
	assert(conn != nullptr);

	const char* pHeaderValue = mg_get_header(conn, headerName.c_str());
	if (pHeaderValue != nullptr)
	{
		return std::make_optional(std::string(pHeaderValue));
	}

	return std::nullopt;
}

HTTP::EHTTPMethod HTTPUtil::GetHTTPMethod(mg_connection* conn)
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

std::optional<Json::Value> HTTPUtil::GetRequestBody(mg_connection* conn)
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
		throw DESERIALIZATION_EXCEPTION_F("Failed to parse json: {}", requestBody);
	}

	return std::make_optional(json);
}

int HTTPUtil::BuildSuccessResponseJSON(mg_connection* conn, const Json::Value& json)
{
	assert(conn != nullptr);

	Json::StreamWriterBuilder builder;
	builder["indentation"] = ""; // Removes whitespaces
	const std::string output = Json::writeString(builder, json);
	return HTTPUtil::BuildSuccessResponse(conn, output);
}

int HTTPUtil::BuildSuccessResponse(mg_connection* conn, const std::string& response)
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

int HTTPUtil::BuildBadRequestResponse(mg_connection* conn, const std::string& response)
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

int HTTPUtil::BuildConflictResponse(mg_connection* conn, const std::string& response)
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

int HTTPUtil::BuildUnauthorizedResponse(mg_connection* conn, const std::string& response)
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

int HTTPUtil::BuildNotFoundResponse(mg_connection* conn, const std::string& response)
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

int HTTPUtil::BuildInternalErrorResponse(mg_connection* conn, const std::string& response)
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