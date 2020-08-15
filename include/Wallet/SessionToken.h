#pragma once

#include <Common/Secure.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Serialization/ByteBuffer.h>
#include <cppcodec/base64_rfc4648.hpp>
#include <cstdint>
#include <string>
#include <vector>

class SessionToken
{
public:
	SessionToken(const uint64_t sessionId, std::vector<unsigned char>&& tokenKey)
		: m_sessionId(sessionId), m_tokenKey(std::move(tokenKey)) { }
	SessionToken(const uint64_t sessionId, const std::vector<unsigned char>& tokenKey)
		: m_sessionId(sessionId), m_tokenKey(tokenKey) { }
	SessionToken(const uint64_t sessionId, const SecureVector& tokenKey)
		: m_sessionId(sessionId), m_tokenKey(tokenKey.begin(), tokenKey.end()) { }

	SessionToken(SessionToken&& rhs) = default;
	SessionToken(const SessionToken& rhs) = default;

	SessionToken& operator=(const SessionToken& rhs) = default;
	SessionToken& operator=(SessionToken&& rhs) noexcept = default;

	uint64_t GetSessionId() const { return m_sessionId; }
	const std::vector<unsigned char>& GetTokenKey() const { return m_tokenKey; }

	// TODO: Use SecureString
	std::string ToBase64() const
	{
		Serializer serializer;
		serializer.Append<uint64_t>(m_sessionId);
		serializer.AppendByteVector(m_tokenKey);
		const std::vector<unsigned char> serialized = serializer.GetBytes();
		
		return cppcodec::base64_rfc4648::encode(serialized);
	}

	static SessionToken FromBase64(const std::string& encoded)
	{
		const std::vector<unsigned char> serialized = cppcodec::base64_rfc4648::decode(encoded);
		ByteBuffer byteBuffer(serialized);

		const uint64_t sessionId = byteBuffer.ReadU64();
		std::vector<unsigned char> tokenKey = byteBuffer.ReadVector(byteBuffer.GetRemainingSize());

		return SessionToken(sessionId, std::move(tokenKey));
	}

private:
	uint64_t m_sessionId;
	std::vector<unsigned char> m_tokenKey;
};