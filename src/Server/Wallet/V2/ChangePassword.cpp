#include "ChangePassword.h"

ChangePassword::ChangePassword(IWalletManagerPtr pWalletManager)
	: m_pWalletManager(pWalletManager)
{

}

RPC::Response ChangePassword::Handle(const RPC::Request& request) const
{
	if (!request.GetParams().has_value())
	{
		throw DESERIALIZATION_EXCEPTION();
	}

	// TODO: Implement
	return request.BuildResult(Json::Value());
}