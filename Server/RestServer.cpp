#include "RestServer.h"
#include "civetweb/include/civetweb.h"

#include "BlockAPI.h"

#include <P2PServer.h>
#include <BlockChainServer.h>
#include <Database.h>

RestServer::RestServer(const Config& config, IDatabase* pDatabase, IBlockChainServer* pBlockChainServer, IP2PServer* pP2PServer)
	: m_config(config), m_pDatabase(pDatabase), m_pBlockChainServer(pBlockChainServer), m_pP2PServer(pP2PServer)
{

}

bool RestServer::Start()
{
	/* Initialize the library */
	mg_init_library(0);

	/* Start the server */
	ctx = mg_start(NULL, 0, NULL);

	/* Add handlers */
	mg_set_request_handler(ctx, "/v1/headers/", BlockAPI::GetHeader_Handler, m_pBlockChainServer);

	return true;
}

bool RestServer::Shutdown()
{
	/* Stop the server */
	mg_stop(ctx);

	/* Un-initialize the library */
	mg_exit_library();

	return true;
}