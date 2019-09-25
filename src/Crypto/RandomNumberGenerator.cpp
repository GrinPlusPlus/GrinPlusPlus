#include <Crypto/RandomNumberGenerator.h>

#include <random>
#include <Crypto/CryptoException.h>

#ifdef _WIN32
#pragma comment(lib, "Bcrypt.lib")
#include <bcrypt.h>
#else
#include <fcntl.h>  
#endif

CBigInteger<32> RandomNumberGenerator::GenerateRandom32()
{
	const SecureVector randomBytes = GenerateRandomBytes(32);
	return CBigInteger<32>(randomBytes.data());
}

SecureVector RandomNumberGenerator::GenerateRandomBytes(const size_t numBytes)
{
	SecureVector buffer(numBytes);
#ifdef _WIN32
	const NTSTATUS status = BCryptGenRandom(nullptr, buffer.data(), buffer.size(), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
	if (!BCRYPT_SUCCESS(status))
	{
		throw CryptoException("RNG Failure");
	}
#else
	bool success = false;
	int hFile = open("/dev/urandom", O_RDONLY);
	if (hFile >= 0)
	{
		if (read(hFile, buffer.data(), numBytes) == numBytes)
		{
			success = true;
		}

		close(hFile);
	}

	if (!success)
	{
		throw CryptoException("RNG Failure");
	}
#endif

	return buffer;
}

uint64_t RandomNumberGenerator::GenerateRandom(const uint64_t minimum, const uint64_t maximum)
{
	std::random_device seeder;
	std::mt19937 engine(seeder());
	std::uniform_int_distribution<uint64_t> dist(minimum, maximum);

	return dist(engine);
}