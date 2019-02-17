#pragma once

#include <Crypto/BigInteger.h>
#include <Wallet/EncryptedSeed.h>
#include <Common/Secure.h>
#include <vector>
#include <memory>

//
// A container for the HD Wallet's master seed and other sensitive data.
// The information is stored in an encrypted form, and requires the user's password to retrieve any usable data from it.
//
// The data is currently stored and manipulated in the following way:
//	* Variables Used: 
//		- P = Plaintext password string
//		- P_hash = Scrypt hash of P
//		- Sa = Randomly generated salt
//		- S = 32 byte wallet seed
//		- S_hash = 32 byte HMAC_SHA256 hash with key=P_hash and data=S
//		- E = 64 byte EncryptedWalletKeys
//		- C = 65 byte combined S and S_hash
//
//	* Functions: 
//		- SCRYPT(stringToHash) = byte[] (Always 64 bytes)
//		- HMAC_SHA256(byte[] key, byte[] dataToHash) = byte[] (Always 32 bytes)
//		- AES256_ENC(byte[] key, byte[] dataToEncrypt) = byte[] (Length of data padded to be a multiple of 32 bytes)
//		- AES256_DEC(byte[] key, byte[] dataToDecrypt) = byte[] (Length of data that was originally encrypted)
//		- CONCAT(byte[] value1, byte[] value2) = byte[] (Length of value1 + length of value2)
//		- FIRST_BYTES(byte[] data, int numBytes) = byte[] (Length = numBytes)
//		- LAST_BYTES(byte[] data, int numBytes) = byte[] (Length = numBytes)
//
//	* Encryption Steps - E = ENC(S, P): 
//		1. P_hash = SCRYPT(P, Sa)
//		2. S_hash = HMAC_SHA256(P_hash, S)
//		3. C = CONCAT(S, S_hash)
//		4. E = AES256_ENC(P_hash, C)
//
//	* Decryption Steps - S = DEC(E, P):
//		1. P_hash = SCRYPT(P, Sa)
//		2. C = AES256_DEC(P_hash, E)
//		3. S = FIRST_BYTES(C, 32)
//		4. S_hash = HMAC_SHA256(P_hash, S)
//		5. If S_hash == LAST_BYTES(C, 32), return S. Otherwise, decryption failed.
//
class SeedEncrypter
{
public:
	//
	// Encrypts the wallet seed using the password and a randomly generated salt.
	//
	EncryptedSeed EncryptWalletSeed(const CBigInteger<32>& walletSeed, const SecureString& password) const;

	//
	// Decrypts the wallet seed using the password and salt given.
	//
	CBigInteger<32> DecryptWalletSeed(const EncryptedSeed& encryptedSeed, const SecureString& password) const;

private:
	 CBigInteger<32> CalculatePasswordHash(const SecureString& password, const CBigInteger<8>& salt) const;
};