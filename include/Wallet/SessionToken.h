#pragma once

#include <stdint.h>
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

private:
	uint64_t m_sessionId;
	std::vector<unsigned char> m_tokenKey;
};