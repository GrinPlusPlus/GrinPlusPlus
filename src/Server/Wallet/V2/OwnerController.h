#pragma once

#include <Config/Config.h>
#include <Wallet/WalletManager.h>
#include <civetweb/include/civetweb.h>

class OwnerController
{
public:
	~OwnerController();

	static std::shared_ptr<OwnerController> Create(const Config& config, IWalletManagerPtr pWalletManager);

private:
	struct CallbackData
	{
		CallbackData(const Config& config, IWalletManagerPtr pWalletManager)
			: m_config(config), m_pWalletManager(pWalletManager)
		{

		}

		const Config& m_config;
		IWalletManagerPtr m_pWalletManager;
	};

	OwnerController(std::shared_ptr<CallbackData> pCallbackData, mg_context* pCivetContext);

	static int OwnerAPIHandler(mg_connection* pConnection, void* pContext);

	std::shared_ptr<CallbackData> m_pCallbackData;
	mg_context* m_pCivetContext;
};