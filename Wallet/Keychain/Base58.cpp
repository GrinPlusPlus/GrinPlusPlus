#include "Base58.h"

#include <Crypto/Crypto.h>
#include <stdexcept>

static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

std::string CBase58::EncodeBase58(const CBigInteger<25>& input)
{
	std::string output;

	CBigInteger<25> remaining = input;
	while (remaining > CBigInteger<25>::ValueOf(0))
	{
		const int remainder = remaining % 58;

		output += pszBase58[remainder];

		remaining = remaining / 58;
	}

	std::reverse(output.begin(), output.end());
	return output;
}

/*
 * Base58 Check:
 * 1. Take the version byte and payload bytes, and concatenate them together (bytewise).
 * 2. Take the first four bytes of SHA256(SHA256(results of step 1))
 * 3. Concatenate the results of step 1 and the results of step 2 together (bytewise).
 * 4. Treating the results of step 3 - a series of bytes - as a single big-endian bignumber, convert to base-58 using normal mathematical steps (bignumber division) and the base-58 alphabet described below. The result should be normalized to not have any leading base-58 zeroes (character '1').
 * 5. The leading character '1', which has a value of zero in base58, is reserved for representing an entire leading zero byte, as when it is in a leading position, has no value as a base-58 symbol. There can be one or more leading '1's when necessary to represent one or more leading zero bytes. Count the number of leading zero bytes that were the result of step 3 (for old Bitcoin addresses, there will always be at least one for the version/application byte; for new addresses, there will never be any). Each leading zero byte shall be represented by its own character '1' in the final result.
 * 6. Concatenate the 1's from step 5 with the results of step 4. This is the Base58Check result.
*/
std::string CBase58::EncodeBase58Check(const CBigInteger<20>& input) // TODO: Include version
{
	// 1. Take the version byte and payload bytes, and concatenate them together (bytewise).
	const std::vector<unsigned char>& payloadBytes = input.GetData();
	std::vector<unsigned char> versionPlusPayload;
	versionPlusPayload.push_back(0); // Version == 1 (binary 0) - 
	versionPlusPayload.insert(versionPlusPayload.end(), payloadBytes.cbegin(), payloadBytes.cend());

	// 2. Take the first four bytes of SHA256(SHA256(results of step 1))
	CBigInteger<32> hash = CCrypto::SHA256d(versionPlusPayload);
	const unsigned char byte1 = hash.GetData().at(0);
	const unsigned char byte2 = hash.GetData().at(1);
	const unsigned char byte3 = hash.GetData().at(2);
	const unsigned char byte4 = hash.GetData().at(3);

	// 3. Concatenate the results of step 1 and the results of step 2 together (bytewise).
	std::vector<unsigned char> versionPlusPayloadPlusHash;
	versionPlusPayloadPlusHash.insert(versionPlusPayloadPlusHash.begin(), versionPlusPayload.cbegin(), versionPlusPayload.cend());
	versionPlusPayloadPlusHash.insert(versionPlusPayloadPlusHash.cend(), hash.GetData().cbegin(), hash.GetData().cbegin() + 4);

	// 4. Treating the results of step 3 - a series of bytes - as a single big-endian bignumber, convert to base-58 using normal mathematical steps (bignumber division) and the base-58 alphabet described below. 
	// The result should be normalized to not have any leading base-58 zeroes (character '1').
	CBigInteger<25> bitcoinAddress(&versionPlusPayloadPlusHash[0]);
	std::string base58EncodedWithoutLeadingOnes = EncodeBase58(bitcoinAddress);

	// 5. The leading character '1', which has a value of zero in base58, is reserved for representing an entire leading zero byte, as when it is in a leading position, has no value as a base-58 symbol.
	// There can be one or more leading '1's when necessary to represent one or more leading zero bytes. 
	// Count the number of leading zero bytes that were the result of step 3 (for old Bitcoin addresses, there will always be at least one for the version/application byte; for new addresses, there will never be any).
	// Each leading zero byte shall be represented by its own character '1' in the final result.
	std::string leadingOnes = "";
	int i = 0;
	while (versionPlusPayload[i] == 0)
	{
		leadingOnes += "1";
		i++;
	}

	// 6. Concatenate the 1's from step 5 with the results of step 4. This is the Base58Check result.

	return leadingOnes + base58EncodedWithoutLeadingOnes;
}

CBigInteger<25> CBase58::DecodeBase58(const std::string& input)
{
	CBigInteger<25> output = CBigInteger<25>::ValueOf(DecodeChar(input[0]));

	const size_t length = input.length();
	for (size_t i = 1; i < length; i++)
	{
		output = output * 58;
		output = output + CBigInteger<25>::ValueOf(DecodeChar(input[i]));
	}

	return output;
}

CBigInteger<20> CBase58::DecodeBase58Check(const std::string& input)
{
	CBigInteger<25> decodedWithCheck = DecodeBase58(input);
	const std::vector<unsigned char>& decodedWithCheckVector = decodedWithCheck.GetData();
	std::vector<unsigned char> decodedWithVersionVector(decodedWithCheckVector.cbegin(), decodedWithCheckVector.cend() - 4);

	CBigInteger<32> expectedHash = CCrypto::SHA256d(decodedWithVersionVector);
	const std::vector<unsigned char>& expectedHashVector = expectedHash.GetData();

	std::vector<unsigned char> expectedHash4Bytes(expectedHashVector.cbegin(), expectedHashVector.cbegin() + 4);
	std::vector<unsigned char> actualHash4Bytes(decodedWithCheckVector.cend() - 4, decodedWithCheckVector.cend());


	if (expectedHash4Bytes != actualHash4Bytes)
	{
		return CBigInteger<20>::ValueOf(0);
	}

	return CBigInteger<20>(&decodedWithVersionVector[1]); // Start at 1, since 0 is version byte
}

// TODO: Quick and dirty solution. Can easily be made more efficient.
int CBase58::DecodeChar(wchar_t input)
{
	for (int i = 0; i < 58; i++)
	{
		if (input == pszBase58[i])
		{
			return i;
		}
	}

	throw std::range_error("Not in Base58 Range!");
}