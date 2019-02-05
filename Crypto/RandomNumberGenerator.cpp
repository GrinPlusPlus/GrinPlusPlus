#include <Crypto/RandomNumberGenerator.h>

#include <random>

CBigInteger<32> RandomNumberGenerator::GenerateRandom32()
{
	// TODO: Find a good CSPRNG
	std::random_device randomDevice;
	std::mt19937 rng(randomDevice());
	std::uniform_int_distribution<unsigned short> uniformDistribution(0, 255);

	std::vector<unsigned char> buffer(32);
	for (uint8_t i = 0; i < 32; i++)
	{
		buffer[i] = (unsigned char)uniformDistribution(rng);
	}

	return CBigInteger<32>(buffer);
}

uint64_t RandomNumberGenerator::GeneratePseudoRandomNumber(const uint64_t minimum, const uint64_t maximum)
{
	std::random_device seeder;
	std::mt19937 engine(seeder());
	std::uniform_int_distribution<uint64_t> dist(minimum, maximum);

	return dist(engine);
}