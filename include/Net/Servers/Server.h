#pragma once

#include <cassert>
#include <string>
#include <optional>
#include <memory>

enum class EServerType
{
	LOCAL,
	PUBLIC
};

// Forward Declarations
struct mg_context;
struct mg_connection;
typedef int(*mg_request_handler)(struct mg_connection* conn, void* cbdata);

class Server
{
public:
	static std::shared_ptr<Server> Create(const EServerType type, const std::optional<uint16_t>& port);
	virtual ~Server();

	uint16_t GetPortNumber() const noexcept { return m_portNumber; }

	void AddListener(const std::string& uri, mg_request_handler handler, void* pCallbackData) noexcept;

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