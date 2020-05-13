#pragma once

#include <Infrastructure/Logger.h>
#include <Net/Clients/HTTP/HTTPException.h>
#include <Common/Util/StringUtil.h>
#include <cassert>
#include <civetweb.h>
#include <string>
#include <optional>
#include <memory>

enum class EServerType
{
	LOCAL,
	PUBLIC
};

class Server
{
public:
	static std::shared_ptr<Server> Create(const EServerType type, const std::optional<uint16_t>& port)
	{
		std::string listenerAddr = type == EServerType::LOCAL ? "127.0.0.1" : "0.0.0.0";
		std::string listeningPort = StringUtil::Format("{}:{}", listenerAddr, port.value_or(0));

		const char* pOptions[] = {
			"num_threads", "15",
			"listening_ports", listeningPort.c_str(),
			NULL
		};

		mg_init_library(0);
		auto pCivetContext = mg_start(NULL, 0, pOptions);
		if (pCivetContext == nullptr)
		{
			LOG_ERROR("Failed to start server.");
			throw HTTP_EXCEPTION("mg_start failed.");
		}

		mg_server_ports ports;
		if (mg_get_server_ports(pCivetContext, 1, &ports) != 1)
		{
			mg_stop(pCivetContext);
			LOG_ERROR("mg_get_server_ports failed.");
			throw HTTP_EXCEPTION("mg_get_server_ports failed.");
		}

		return std::shared_ptr<Server>(new Server(pCivetContext, (uint16_t)ports.port));
	}

	virtual ~Server()
	{
		try
		{
			mg_stop(m_pContext);
			mg_exit_library();
		}
		catch (const std::system_error& e)
		{
			LOG_ERROR_F("Exception thrown while stopping node API listener: {}", e.what());
		}
	}

	uint16_t GetPortNumber() const noexcept { return m_portNumber; }

	void AddListener(const std::string& uri, mg_request_handler handler, void* pCallbackData) noexcept
	{
		mg_set_request_handler(m_pContext, uri.c_str(), handler, pCallbackData);
	}

private:
	Server(mg_context* pContext, const uint16_t portNumber)
		: m_pContext(pContext), m_portNumber(portNumber)
	{
		assert(pContext != nullptr);
		assert(portNumber > 0);
	}

	mg_context* m_pContext;
	uint16_t m_portNumber;
};

typedef std::shared_ptr<Server> ServerPtr;