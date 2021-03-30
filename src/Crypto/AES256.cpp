#include <Crypto/AES256.h>

#include <bitcoin/aes.h>
#include <Core/Exceptions/CryptoException.h>

std::vector<uint8_t> AES256::Encrypt(const SecureVector& input, const SecretKey& key, const CBigInteger<16>& iv)
{   
	std::vector<uint8_t> ciphertext;

	// max ciphertext len for a n bytes of plaintext is n + AES_BLOCKSIZE bytes
	ciphertext.resize(input.size() + AES_BLOCKSIZE);

	AES256CBCEncrypt enc(key.data(), iv.data(), true);
	const size_t nLen = enc.Encrypt(&input[0], (int)input.size(), ciphertext.data());
	if (nLen < input.size()) {
		throw CRYPTO_EXCEPTION("Failed to encrypt");
	}

	ciphertext.resize(nLen);

	return ciphertext ;
}

SecureVector AES256::Decrypt(const std::vector<uint8_t>& ciphertext, const SecretKey& key, const CBigInteger<16>& iv)
{    
	SecureVector plaintext;

	// plaintext will always be equal to or lesser than length of ciphertext
	size_t nLen = ciphertext.size();

	plaintext.resize(nLen);

	AES256CBCDecrypt dec(key.data(), iv.data(), true);
	nLen = dec.Decrypt(ciphertext.data(), (int)ciphertext.size(), plaintext.data());
	if (nLen == 0) {
		throw CRYPTO_EXCEPTION("Failed to decrypt");
	}

	plaintext.resize(nLen);

	return plaintext;
}