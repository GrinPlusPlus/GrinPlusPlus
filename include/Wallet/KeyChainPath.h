#pragma once

#include <Core/Traits/Printable.h>
#include <Core/Exceptions/DeserializationException.h>
#include <Crypto/CSPRNG.h>
#include <cstdint>
#include <string>
#include <vector>

static const uint32_t MINIMUM_HARDENED_INDEX = 2147483648;

class KeyChainPath : public Traits::IPrintable
{
public:
	KeyChainPath(std::vector<uint32_t>&& keyIndices)
		: m_keyIndices(keyIndices)
	{

	}

	virtual ~KeyChainPath() = default;

	//
	// Operators
	//
	bool operator<(const KeyChainPath& other) const { return GetKeyIndices() < other.GetKeyIndices(); }
	bool operator==(const KeyChainPath& other) const { return GetKeyIndices() == other.GetKeyIndices(); }
	bool operator!=(const KeyChainPath& other) const { return GetKeyIndices() != other.GetKeyIndices(); }

	const std::vector<uint32_t>& GetKeyIndices() const { return m_keyIndices; }

	std::string Format() const final
	{
		std::string path = "m";
		for (const uint32_t keyIndex : m_keyIndices)
		{
			path += "/" + std::to_string(keyIndex); // TODO: Handle hardened?
		}

		return path;
	}

	static KeyChainPath FromHex(const std::string& hex)
	{
		std::vector<uint32_t> keyIndices;

		ByteBuffer buffer(HexUtil::FromHex(hex));
		const uint8_t size = buffer.ReadU8();
		if (size > 4)
		{
			throw DESERIALIZATION_EXCEPTION_F("Hex too large: {}", hex);
		}

		for (uint8_t i = 0; i < size; i++)
		{
			keyIndices.push_back(buffer.ReadU32());
		}

		return KeyChainPath(std::move(keyIndices));
	}

	std::string ToHex() const noexcept
	{
		Serializer serializer;
		serializer.Append<uint8_t>((uint8_t)m_keyIndices.size());
		serializer.Append<uint32_t>(m_keyIndices.size() > 0 ? m_keyIndices[0] : 0);
		serializer.Append<uint32_t>(m_keyIndices.size() > 1 ? m_keyIndices[1] : 0);
		serializer.Append<uint32_t>(m_keyIndices.size() > 2 ? m_keyIndices[2] : 0);
		serializer.Append<uint32_t>(m_keyIndices.size() > 3 ? m_keyIndices[3] : 0);

		return HexUtil::ConvertToHex(serializer.GetBytes());
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
		keyIndicesCopy.push_back((uint32_t)CSPRNG::GenerateRandom(0, MINIMUM_HARDENED_INDEX - 1000000));
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
			throw DESERIALIZATION_EXCEPTION_F("Invalid path: {}", path);
		}

		std::vector<uint32_t> keyIndices;

		std::string remaining = path.substr(1);
		while (remaining.size() >= 2)
		{
			if (remaining.at(0) != '/')
			{
				throw DESERIALIZATION_EXCEPTION_F("Invalid path: {}", path);
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