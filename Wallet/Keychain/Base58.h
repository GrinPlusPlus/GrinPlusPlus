#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <BigInteger.h>
#include <string>

// TODO: Use generic template so sizes aren't fixed?
class CBase58
{
public:
	std::string EncodeBase58Check(const CBigInteger<20>& input);

	CBigInteger<20> DecodeBase58Check(const std::string& input);

private:
	std::string EncodeBase58(const CBigInteger<25>& input);
	CBigInteger<25> DecodeBase58(const std::string& input);
	int DecodeChar(wchar_t input);
};