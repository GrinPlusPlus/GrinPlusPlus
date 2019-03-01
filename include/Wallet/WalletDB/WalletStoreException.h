#pragma once

#include <exception>

class WalletStoreException : public std::exception
{
public:
	const char* what() const throw()
	{
		return "Wallet Store Exception.";
	}
};