#include "OwnerFinalize.h"

#include <Net/Tor/TorManager.h>
#include <Net/Tor/TorAddressParser.h>
#include <Common/Util/FileUtil.h>

OwnerFinalize::OwnerFinalize(IWalletManagerPtr pWalletManager)
	: m_pWalletManager(pWalletManager)
{

}

RPC::Response OwnerFinalize::Handle(const RPC::Request& request) const
{
	if (!request.GetParams().has_value())
	{
		throw DESERIALIZATION_EXCEPTION();
	}

	FinalizeCriteria criteria = FinalizeCriteria::FromJSON(request.GetParams().value());

	if (criteria.GetFile().has_value())
	{
		return FinalizeViaFile(request, criteria, criteria.GetFile().value());
	}
	else
	{
		Slate slate = m_pWalletManager->Finalize(criteria);

		Json::Value result;
		result["status"] = "FINALIZED";
		result["slate"] = slate.ToJSON();
		return request.BuildResult(result);
	}
}

RPC::Response OwnerFinalize::FinalizeViaFile(const RPC::Request& request, const FinalizeCriteria& criteria, const std::string& file) const
{
	// FUTURE: Check write permissions before creating slate

	Slate slate = m_pWalletManager->Finalize(criteria);

	Json::Value slateJSON = slate.ToJSON();

	try
	{
		FileUtil::WriteTextToFile(file, JsonUtil::WriteCondensed(slateJSON));
		WALLET_INFO_F("Slate file saved to: {}", file);

		Json::Value result;
		result["status"] = "FINALIZED";
		result["slate"] = slateJSON;
		return RPC::Response::BuildResult(request.GetId(), result);
	}
	catch (std::exception&)
	{
		WALLET_ERROR_F("Slate failed to save to: {}", file);

		Json::Value errorJson;
		errorJson["status"] = "WRITE_FAILED";
		errorJson["slate"] = slateJSON;
		return request.BuildError(RPC::ErrorCode::INTERNAL_ERROR, "Failed to write file.", errorJson);
	}
}