#pragma once

#include <Config/Config.h>
#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <optional>

class ChangePasswordHandler : public RPCMethod
{
public:
	ChangePasswordHandler(const IWalletManagerPtr& pWalletManager)
		: m_pWalletManager(pWalletManager) { }
	virtual ~ChangePasswordHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		// TODO: Implement
		return request.BuildResult(Json::Value());
	}

	bool ContainsSecrets() const noexcept final { return true; }

private:
	IWalletManagerPtr m_pWalletManager;
};