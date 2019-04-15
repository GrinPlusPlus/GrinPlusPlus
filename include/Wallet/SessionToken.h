#pragma once

#include <Common/Base64.h>
#include <Common/Secure.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Serialization/ByteBuffer.h>
#include <stdint.h>
#include <string>
#include <vector>

class SessionToken
{
public:
	SessionToken(const uint64_t sessionId, std::vector<unsigned char>&& tokenKey)
		: m_sessionId(sessionId), m_tokenKey(std::move(tokenKey))
	{

	}

	inline uint64_t GetSessionId() const { return m_sessionId; }
	inline const std::vector<unsigned char>& GetTokenKey() const { return m_tokenKey; }

	// TODO: Use SecureString
	std::string ToBase64() const
	{
		Serializer serializer;
		serializer.Append<uint64_t>(m_sessionId);
		serializer.AppendByteVector(m_tokenKey);
		const std::vector<unsigned char> serialized = serializer.GetBytes();
		
		return Base64::EncodeBase64(serialized);
	}

	static SessionToken FromBase64(const std::string& encoded)
	{
		const std::vector<unsigned char> serialized = Base64::DecodeBase64(encoded);
		ByteBuffer byteBuffer(serialized);

		const uint64_t sessionId = byteBuffer.ReadU64();
		std::vector<unsigned char> tokenKey = byteBuffer.ReadVector(byteBuffer.GetRemainingSize());

		return SessionToken(sessionId, std::move(tokenKey));
	}

private:
	uint64_t m_sessionId;
	std::vector<unsigned char> m_tokenKey;
};