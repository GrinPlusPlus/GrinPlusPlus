#pragma once

#include <exception>
#include <optional>
#include <json/json.h>

#define RPC_EXCEPTION(msg, id) RPCException(__func__, msg, id)

class RPCException : public std::exception
{
public:
	RPCException(const std::string& function, const std::string& message, const std::optional<Json::Value>& idOpt)
		: std::exception()
	{
		m_message = message;
		m_idOpt = idOpt;
		m_what = function + " - " + message;
	}

	RPCException() : std::exception()
	{
		m_message = "RPC Exception";
	}

	virtual const char* what() const throw()
	{
		return m_what.c_str();
	}

	const std::optional<Json::Value>& GetId() const noexcept { return m_idOpt; }
	const std::string& GetMsg() const noexcept { return m_message; }

private:
	std::string m_message;
	std::string m_what;
	std::optional<Json::Value> m_idOpt;
};