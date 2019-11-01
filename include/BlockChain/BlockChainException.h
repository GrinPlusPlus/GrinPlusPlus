#pragma once

#include <stdexcept>
#include <string>

#define BLOCK_CHAIN_EXCEPTION(msg) BlockChainException(msg, __FUNCTION__)

class BlockChainException : public std::exception
{
public:
	BlockChainException(const std::string& message, const std::string& function)
	{
		m_message = message;
		m_function = function;
		m_what = function + ": " + message;
	}

	virtual const char* what() const throw()
	{
		return m_what.c_str();
	}

private:
	std::string m_message;
	std::string m_function;
	std::string m_what;
};