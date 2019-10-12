#pragma once

#include <string>

namespace HTTP
{
enum class EHTTPMethod
{
	GET,
	POST
};

class Request
{
public:
	static Request BuildRequest(const EHTTPMethod method, const std::string& body)
	{
		// TODO: Implement
		return Request();
	}

	std::string ToString() const
	{
		// TODO: Implement
		return "\r\n\r\n";
	}
};

class Response
{

};
}