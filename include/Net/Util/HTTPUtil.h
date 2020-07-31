#pragma once

#include <Net/Clients/HTTP/HTTP.h>
#include <json/json.h>
#include <cassert>
#include <string>
#include <optional>

// Forward Declarations
struct mg_connection;

class HTTPUtil
{
public:
	// Strips away the base URI and any query strings.
	// Ex: Given "/v1/blocks/<hash>?compact" and baseURI "/v1/blocks/", this would return <hash>.
	static std::string GetURIParam(mg_connection* conn, const std::string& baseURI);
	static std::string GetQueryString(mg_connection* conn) noexcept;
	static bool HasQueryParam(mg_connection* conn, const std::string& parameterName);
	static std::optional<std::string> GetQueryParam(mg_connection* conn, const std::string& parameterName);
	static std::optional<std::string> GetHeaderValue(mg_connection* conn, const std::string& headerName);
	static HTTP::EHTTPMethod GetHTTPMethod(mg_connection* conn);
	static std::optional<Json::Value> GetRequestBody(mg_connection* conn);

	static int BuildSuccessResponseJSON(mg_connection* conn, const Json::Value& json);
	static int BuildSuccessResponse(mg_connection* conn, const std::string& response);
	static int BuildBadRequestResponse(mg_connection* conn, const std::string& response);
	static int BuildConflictResponse(mg_connection* conn, const std::string& response);
	static int BuildUnauthorizedResponse(mg_connection* conn, const std::string& response);
	static int BuildNotFoundResponse(mg_connection* conn, const std::string& response);
	static int BuildInternalErrorResponse(mg_connection* conn, const std::string& response);
};