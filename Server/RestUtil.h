#pragma once

#include "civetweb/include/civetweb.h"

#include <string>

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

	static int BuildSuccessResponse(struct mg_connection* conn, const std::string& response)
	{
		unsigned long len = (unsigned long)response.size();

		mg_printf(conn,
			"HTTP/1.1 200 OK\r\n"
			"Content-Length: %lu\r\n"
			"Content-Type: application/json\r\n"
			"Connection: close\r\n\r\n",
			len);

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
};