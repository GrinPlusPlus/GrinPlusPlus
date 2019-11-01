#pragma once

#include "NodeContext.h"
#include <Config/Config.h>

// Forward Declarations
struct mg_context;

class NodeRestServer
{
public:
	NodeRestServer(const Config& config, std::shared_ptr<NodeContext> pNodeContext);
	~NodeRestServer();

	bool Initialize();
	bool Shutdown();

private:
	const Config& m_config;

	std::shared_ptr<NodeContext> m_pNodeContext;
	mg_context* m_pNodeCivetContext;
};