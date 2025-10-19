#pragma once

#include <Core/Config.h>
#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <optional>

class GetTopLevelDirectoryHandler : public RPCMethod
{
public:
	GetTopLevelDirectoryHandler(const IWalletManagerPtr& pWalletManager)
		: m_pWalletManager(pWalletManager) { }
	virtual ~GetTopLevelDirectoryHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		auto directory = m_pWalletManager->GetWalletsDirectory();
		Json::Value result;
		result["Ok"] = directory;
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return true; }

private:
	IWalletManagerPtr m_pWalletManager;
};