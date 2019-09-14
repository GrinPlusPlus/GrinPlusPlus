#include <Crypto/RandomNumberGenerator.h>

#include <random>

CBigInteger<32> RandomNumberGenerator::GenerateRandom32()
{
	const SecureVector randomBytes = GenerateRandomBytes(32);
	return CBigInteger<32>(randomBytes.data());
}

SecureVector RandomNumberGenerator::GenerateRandomBytes(const size_t numBytes)
{
	// TODO: Find a good CSPRNG
	std::random_device randomDevice;
	std::mt19937 rng(randomDevice());
	std::uniform_int_distribution<unsigned short> uniformDistribution(0, 255);

	SecureVector buffer(numBytes);
	for (uint8_t i = 0; i < numBytes; i++)
	{
		buffer[i] = (unsigned char)uniformDistribution(rng);
	}

	return buffer;
}

uint64_t RandomNumberGenerator::GenerateRandom(const uint64_t minimum, const uint64_t maximum)
{
	std::random_device seeder;
	std::mt19937 engine(seeder());
	std::uniform_int_distribution<uint64_t> dist(minimum, maximum);

	return dist(engine);
}