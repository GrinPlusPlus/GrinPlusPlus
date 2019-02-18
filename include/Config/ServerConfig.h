#pragma once

#include <stdint.h>

class ServerConfig
{
public:
	ServerConfig(const uint32_t restAPIPort)
		: m_restAPIPort(restAPIPort)
	{

	}

	inline uint32_t GetRestAPIPort() const { return m_restAPIPort; }

private:
	uint32_t m_restAPIPort;
};