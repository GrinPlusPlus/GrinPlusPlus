#pragma once

#include <Serialization/DeserializationException.h>
#include <stdint.h>
#include <string>
#include <vector>

class KeyChainPath
{
public:
	KeyChainPath(std::vector<uint32_t>&& keyIndices)
		: m_keyIndices(keyIndices)
	{

	}

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