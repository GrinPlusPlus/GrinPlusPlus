#pragma once

#include <exception>

class InvalidMnemonicException : public std::exception
{
public:
	const char* what() const throw()
	{
		return "Invalid Mnemonic Provided.";
	}
};