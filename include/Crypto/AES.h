#pragma once

#include <Crypto/BigInteger.h>
#include <Crypto/SecretKey.h>
#include <Common/Secure.h>

class AES
{
public:
	//
	// Encrypts the input with AES256 using the given key.
	//
	static std::vector<uint8_t> AES256_Encrypt(
		const SecureVector& input,
		const SecretKey& key,
		const CBigInteger<16>& iv
	);

	//
	// Decrypts the input with AES256 using the given key.
	//
	static SecureVector AES256_Decrypt(
		const std::vector<uint8_t>& ciphertext,
		const SecretKey& key,
		const CBigInteger<16>& iv
	);    
};