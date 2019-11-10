#pragma once

#include "NodeContext.h"
#include <Config/Config.h>

// Forward Declarations
struct mg_context;

class NodeRestServer
{
public:
	static std::shared_ptr<NodeRestServer> Create(const Config& config, std::shared_ptr<NodeContext> pNodeContext);
	~NodeRestServer();

private:
	NodeRestServer(const Config& config, std::shared_ptr<NodeContext> pNodeContext);

	void Initialize();

	const Config& m_config;

	std::shared_ptr<NodeContext> m_pNodeContext;
	mg_context* m_pNodeCivetContext;
};