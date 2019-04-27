#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Crypto/Crypto.h>
#include <Core/Serialization/DeserializationException.h>

static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

class Base58
{
public:
	template<size_t NUM_BYTES, class ALLOC = std::allocator<unsigned char>>
	std::string EncodeBase58(const CBigInteger<NUM_BYTES, ALLOC>& input)
	{
		std::string output;

		CBigInteger<NUM_BYTES, ALLOC> remaining = input;
		while (remaining > CBigInteger<NUM_BYTES, ALLOC>::ValueOf(0))
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
	std::string EncodeBase58Check(const CBigInteger<33>& input, const std::vector<unsigned char>& versionBytes)
	{
		// 1. Take the version byte and payload bytes, and concatenate them together (bytewise).
		const std::vector<unsigned char>& payloadBytes = input.GetData();
		std::vector<unsigned char> versionPlusPayload = versionBytes;
		versionPlusPayload.insert(versionPlusPayload.end(), payloadBytes.cbegin(), payloadBytes.cend());

		// 2. Take the first four bytes of SHA256(SHA256(results of step 1))
		CBigInteger<32> hash = Crypto::SHA256(Crypto::SHA256(versionPlusPayload).GetData());

		// 3. Concatenate the results of step 1 and the results of step 2 together (bytewise).
		std::vector<unsigned char> versionPlusPayloadPlusHash = versionPlusPayload;
		versionPlusPayloadPlusHash.insert(versionPlusPayloadPlusHash.end(), hash.GetData().cbegin(), hash.GetData().cbegin() + 4);

		// 4. Treating the results of step 3 - a series of bytes - as a single big-endian bignumber, convert to base-58 using normal mathematical steps (bignumber division) and the base-58 alphabet described below. 
		// The result should be normalized to not have any leading base-58 zeroes (character '1').
		CBigInteger<39> grinboxAddress(&versionPlusPayloadPlusHash[0]);
		std::string base58EncodedWithoutLeadingOnes = EncodeBase58(grinboxAddress);

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

	CBigInteger<39> DecodeBase58(const std::string& input)
	{
		CBigInteger<39> output = CBigInteger<39>::ValueOf(DecodeChar(input[0]));

		const size_t length = input.length();
		for (size_t i = 1; i < length; i++)
		{
			output = output * 58;
			output = output + CBigInteger<39>::ValueOf(DecodeChar(input[i]));
		}

		return output;
	}

	CBigInteger<33> DecodeBase58Check(const std::string& input)
	{
		CBigInteger<39> decodedWithCheck = DecodeBase58(input);
		const std::vector<unsigned char>& decodedWithCheckVector = decodedWithCheck.GetData();
		std::vector<unsigned char> decodedWithVersionVector(decodedWithCheckVector.cbegin(), decodedWithCheckVector.cend() - 4);

		CBigInteger<32> expectedHash = Crypto::SHA256(Crypto::SHA256(decodedWithVersionVector).GetData());
		const std::vector<unsigned char>& expectedHashVector = expectedHash.GetData();

		std::vector<unsigned char> expectedHash4Bytes(expectedHashVector.cbegin(), expectedHashVector.cbegin() + 4);
		std::vector<unsigned char> actualHash4Bytes(decodedWithCheckVector.cend() - 4, decodedWithCheckVector.cend());


		if (expectedHash4Bytes != actualHash4Bytes)
		{
			return CBigInteger<33>::ValueOf(0);
		}

		return CBigInteger<33>(&decodedWithVersionVector[2]); // Start at 2, since 0 & 1 are version bytes
	}

private:
	// TODO: Quick and dirty solution. Can easily be made more efficient.
	unsigned char DecodeChar(const wchar_t input)
	{
		for (unsigned char i = 0; i < 58; i++)
		{
			if (input == pszBase58[i])
			{
				return i;
			}
		}

		throw DeserializationException();
	}
};