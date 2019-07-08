#pragma once

#include <exception>

#define WALLET_STORE_EXCEPTION(msg) WalletStoreException(msg, __FUNCTION__)

class WalletStoreException : public std::exception
{
public:
	WalletStoreException(const std::string& message, const std::string& function)
		: std::exception()
	{
		m_message = function + " - " + message;
	}

	WalletStoreException() : std::exception()
	{
		m_message = "Wallet Store Exception";
	}

	virtual const char* what() const throw()
	{
		return m_message.c_str();
	}

private:
	std::string m_message;
};