#pragma once

#include <Crypto/Models/BigInteger.h>
#include <Crypto/Models/SecretKey.h>
#include <Common/Secure.h>

class AES256
{
public:
	//
	// Encrypts the input with AES256 using the given key.
	//
	static std::vector<uint8_t> Encrypt(
		const SecureVector& input,
		const SecretKey& key,
		const CBigInteger<16>& iv
	);

	//
	// Decrypts the input with AES256 using the given key.
	//
	static SecureVector Decrypt(
		const std::vector<uint8_t>& ciphertext,
		const SecretKey& key,
		const CBigInteger<16>& iv
	);    
};