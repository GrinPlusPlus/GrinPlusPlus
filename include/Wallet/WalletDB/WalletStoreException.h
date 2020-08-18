#pragma once

#include <Common/Util/StringUtil.h>
#include <exception>

#define WALLET_STORE_EXCEPTION(msg) WalletStoreException(msg, __func__)
#define WALLET_STORE_EXCEPTION_F(msg, ...) WalletStoreException(StringUtil::Format(msg, __VA_ARGS__), __func__)

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