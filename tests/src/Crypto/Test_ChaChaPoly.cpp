#include <catch.hpp>

#include <Crypto/ChaChaPoly.h>
#include <Crypto/CSPRNG.h>

TEST_CASE("ChaChaPoly - Stream Encryption")
{
	const SecretKey encryption_key = CSPRNG::GenerateRandom32();

	SecureVector random_data = CSPRNG::GenerateRandomBytes(500);
	std::vector<uint8_t> vec(random_data.cbegin(), random_data.cend());
	
	std::vector<uint8_t> encrypted = ChaChaPoly::Init(encryption_key).StreamEncrypt(vec);
	std::vector<uint8_t> decrypted = ChaChaPoly::Init(encryption_key).StreamDecrypt(encrypted);

	REQUIRE(vec == decrypted);
}