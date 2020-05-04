#pragma once

#include <Config/Config.h>
#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <Net/Tor/TorProcess.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <Common/Util/FileUtil.h>
#include <optional>

class FinalizeHandler : public RPCMethod
{
public:
	FinalizeHandler(const TorProcess::Ptr& pTorProcess, const IWalletManagerPtr& pWalletManager)
		: m_pTorProcess(pTorProcess), m_pWalletManager(pWalletManager) { }
	~FinalizeHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		FinalizeCriteria criteria = FinalizeCriteria::FromJSON(request.GetParams().value());

		if (criteria.GetFile().has_value())
		{
			return FinalizeViaFile(request, criteria, criteria.GetFile().value());
		}
		else
		{
			Slate slate = m_pWalletManager->Finalize(criteria, m_pTorProcess);

			Json::Value result;
			result["status"] = "FINALIZED";
			result["slate"] = slate.ToJSON();
			return request.BuildResult(result);
		}
	}

private:
	RPC::Response FinalizeViaFile(
		const RPC::Request& request,
		const FinalizeCriteria& criteria,
		const std::string& file) const
	{
		// FUTURE: Check write permissions before creating slate

		Slate slate = m_pWalletManager->Finalize(criteria, m_pTorProcess);

		Json::Value slateJSON = slate.ToJSON();

		try
		{
			FileUtil::WriteTextToFile(FileUtil::ToPath(file), JsonUtil::WriteCondensed(slateJSON));
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

	TorProcess::Ptr m_pTorProcess;
	IWalletManagerPtr m_pWalletManager;
};