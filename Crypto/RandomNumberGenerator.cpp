#include <Crypto/RandomNumberGenerator.h>

#include <random>

CBigInteger<32> RandomNumberGenerator::GeneratePseudoRandomNumber(const CBigInteger<32>& minimum, const CBigInteger<32>& maximum)
{
	if (maximum <= minimum)
	{
		throw std::exception("RandomNumberGenerator::GeneratePseudoRandomNumber - Maximum value is not greater than minimum value.");
	}

	const CBigInteger<32> differenceBetweenMaxAndMin = maximum - minimum;
	const uint8_t numRandomBytes = DetermineNumberOfRandomBytesNeeded(differenceBetweenMaxAndMin);

	std::vector<unsigned char> randomBytesVector = GeneratePseudoRandomBytes(numRandomBytes);
	CBigInteger<32> randomNumber = ConvertRandomBytesToBigInteger(randomBytesVector) + minimum;

	while (randomNumber > maximum)
	{
		// Obviously, looping is less than ideal. If the difference between maximum and minimum is exactly 2^x, where x is any positive whole integer, 
		// this would take on average 128 tries (due to there being 256 possible values in a byte). Aside from poor performance, that also burns through a great deal
		// of the random number generation seed.
		// TODO: This can be solved using modulos, but I need to think about how to do it in a way that's guaranteed to evenly distribute the possible values.
		// To understand why a simple modulo won't work, consider a minimum value of 0 and a maximum value of 129. Simply modding a random byte (0-255) by 129 gives
		// 2 chances for each value 0-126 but only 1 chance for each value 127-129, ie. an unevely distributed random number.
		randomBytesVector = GeneratePseudoRandomBytes(numRandomBytes);
		randomNumber = ConvertRandomBytesToBigInteger(randomBytesVector) + minimum;
	}

	return randomNumber;
}

uint8_t RandomNumberGenerator::DetermineNumberOfRandomBytesNeeded(const CBigInteger<32>& differenceBetweenMaximumAndMinimum)
{
	uint8_t numRandomBytes = 32;

	const std::vector<unsigned char>& vector = differenceBetweenMaximumAndMinimum.GetData();
	for (int i = 0; i < 32; i++)
	{
		if (vector[i] == 0)
		{
			numRandomBytes--;
		}
		else
		{
			break;
		}
	}

	return numRandomBytes;
}

std::vector<unsigned char> RandomNumberGenerator::GeneratePseudoRandomBytes(const uint8_t numBytes)
{
	// TODO: Find a good CSPRNG
	std::random_device randomDevice;
	std::mt19937 rng(randomDevice());
	std::uniform_int_distribution<unsigned short> uniformDistribution(0, 255);

	std::vector<unsigned char> buffer(numBytes);
	for (uint8_t i = 0; i < numBytes; i++)
	{
		buffer[i] = (unsigned char)uniformDistribution(rng);
	}

	return buffer;
}

CBigInteger<32> RandomNumberGenerator::ConvertRandomBytesToBigInteger(const std::vector<unsigned char>& randomBytes)
{
	// If randomBytes is already 32 bytes, we're done.
	if ((int)randomBytes.size() == 32)
	{
		return CBigInteger<32>(&randomBytes[0]);
	}

	// Otherwise, pad the data with leading 0s.
	std::vector<unsigned char> paddedRandomBytesVector(32);

	const int numberOfLeading0s = 32 - (int)randomBytes.size();
	for (int i = 0; i < 32; i++)
	{
		if (i < numberOfLeading0s)
		{
			paddedRandomBytesVector[i] = 0;
		}
		else
		{
			paddedRandomBytesVector[i] = randomBytes[i - numberOfLeading0s];
		}
	}

	return CBigInteger<32>(&paddedRandomBytesVector[0]);
}