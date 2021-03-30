#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Crypto/Models/BigInteger.h>
#include <Common/Util/BitUtil.h>
#include <Core/Traits/Printable.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

class Commitment : public Traits::IPrintable
{
public:
	//
	// Constructors
	//
	Commitment() = default;
	Commitment(CBigInteger<33>&& commitmentBytes)
		: m_commitmentBytes(std::move(commitmentBytes))
	{

	}
	Commitment(const CBigInteger<33>& commitmentBytes)
		: m_commitmentBytes(commitmentBytes)
	{

	}
	Commitment(const Commitment& other) = default;
	Commitment(Commitment&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~Commitment() = default;

	//
	// Operators
	//
	Commitment& operator=(const Commitment& other) = default;
	Commitment& operator=(Commitment&& other) noexcept = default;
	bool operator<(const Commitment& rhs) const { return m_commitmentBytes < rhs.GetBytes(); }
	bool operator!=(const Commitment& rhs) const { return m_commitmentBytes != rhs.GetBytes(); }
	bool operator==(const Commitment& rhs) const { return m_commitmentBytes == rhs.GetBytes(); }

	//
	// Getters
	//
	const CBigInteger<33>& GetBytes() const noexcept { return m_commitmentBytes; }
	const std::vector<unsigned char>& GetVec() const noexcept { return m_commitmentBytes.GetData(); }
	const unsigned char* data() const noexcept { return m_commitmentBytes.data(); }
	unsigned char* data() noexcept { return m_commitmentBytes.data(); }
	size_t size() const noexcept { return m_commitmentBytes.size(); }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const
	{
		serializer.AppendBigInteger<33>(m_commitmentBytes);
	}

	static Commitment Deserialize(ByteBuffer& byteBuffer)
	{
		return Commitment(byteBuffer.ReadBigInteger<33>());
	}

	std::string ToHex() const
	{
		return m_commitmentBytes.ToHex();
	}

	static Commitment FromHex(const std::string& hex)
	{
		return Commitment(CBigInteger<33>::FromHex(hex));
	}

	//
	// Traits
	//
	std::string Format() const final { return "Commitment{" + m_commitmentBytes.ToHex() + "}"; }

private:
	// The 33 byte commitment.
	CBigInteger<33> m_commitmentBytes;
};

namespace std
{
	template<>
	struct hash<Commitment>
	{
		size_t operator()(const Commitment& commitment) const
		{
			const std::vector<unsigned char>& bytes = commitment.GetVec();
			return BitUtil::ConvertToU64(bytes[0], bytes[4], bytes[8], bytes[12], bytes[16], bytes[20], bytes[24], bytes[28]);
		}
	};
}