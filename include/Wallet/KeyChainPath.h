#pragma once

#include <Core/Serialization/DeserializationException.h>
#include <Crypto/RandomNumberGenerator.h>
#include <stdint.h>
#include <string>
#include <vector>

static const uint32_t MINIMUM_HARDENED_INDEX = 2147483648;

class KeyChainPath
{
public:
	KeyChainPath(std::vector<uint32_t>&& keyIndices)
		: m_keyIndices(keyIndices)
	{

	}

	//
	// Operators
	//
	inline bool operator<(const KeyChainPath& other) const { return GetKeyIndices() < other.GetKeyIndices(); }
	inline bool operator==(const KeyChainPath& other) const { return GetKeyIndices() == other.GetKeyIndices(); }
	inline bool operator!=(const KeyChainPath& other) const { return GetKeyIndices() != other.GetKeyIndices(); }

	const std::vector<uint32_t>& GetKeyIndices() const { return m_keyIndices; }

	std::string ToString() const
	{
		std::string path = "m";
		for (const uint32_t keyIndex : m_keyIndices)
		{
			path += "/" + std::to_string(keyIndex); // TODO: Handle hardened?
		}

		return path;
	}

	KeyChainPath GetFirstChild() const
	{
		std::vector<uint32_t> keyIndicesCopy = m_keyIndices;
		keyIndicesCopy.push_back(0);
		return KeyChainPath(std::move(keyIndicesCopy));
	}

	KeyChainPath GetRandomChild() const
	{
		std::vector<uint32_t> keyIndicesCopy = m_keyIndices;
		keyIndicesCopy.push_back((uint32_t)RandomNumberGenerator::GenerateRandom(0, MINIMUM_HARDENED_INDEX - 1000000));
		return KeyChainPath(std::move(keyIndicesCopy));
	}

	KeyChainPath GetChild(const uint32_t childIndex) const
	{
		std::vector<uint32_t> keyIndicesCopy = m_keyIndices;
		keyIndicesCopy.push_back(childIndex);
		return KeyChainPath(std::move(keyIndicesCopy));
	}

	static KeyChainPath FromString(const std::string& path)
	{
		if (path.empty() || path.at(0) != 'm')
		{
			throw DeserializationException();
		}

		std::vector<uint32_t> keyIndices;

		std::string remaining = path.substr(1);
		while (remaining.size() >= 2)
		{
			if (remaining.at(0) != '/')
			{
				throw DeserializationException();
			}

			remaining = remaining.substr(1);

			size_t sz;
			const uint32_t keyIndex = (uint32_t)std::stoi(remaining, &sz);
			keyIndices.push_back(keyIndex);
			remaining = remaining.substr(sz);
		}

		return KeyChainPath(std::move(keyIndices));
	}

private:
	std::vector<uint32_t> m_keyIndices;
};