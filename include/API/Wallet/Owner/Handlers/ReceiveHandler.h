#pragma once

#include <Config/Config.h>
#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <Net/Tor/TorManager.h>
#include <Net/Tor/TorAddressParser.h>
#include <Common/Util/FileUtil.h>
#include <optional>

class ReceiveHandler : public RPCMethod
{
public:
	ReceiveHandler(const IWalletManagerPtr& pWalletManager)
		: m_pWalletManager(pWalletManager) { }
	virtual ~ReceiveHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			throw DESERIALIZATION_EXCEPTION();
		}

		ReceiveCriteria criteria = ReceiveCriteria::FromJSON(request.GetParams().value());

		if (criteria.GetFile().has_value())
		{
			return ReceiveViaFile(request, criteria, criteria.GetFile().value());
		}
		else
		{
			Slate slate = m_pWalletManager->Receive(criteria);

			Json::Value result;
			result["status"] = "RECEIVED";
			result["slate"] = slate.ToJSON();
			return request.BuildResult(result);
		}
	}

private:
	RPC::Response ReceiveViaFile(
		const RPC::Request& request,
		const ReceiveCriteria& criteria,
		const std::string& file) const
	{
		// FUTURE: Check write permissions before creating slate

		Slate slate = m_pWalletManager->Receive(criteria);

		Json::Value slateJSON = slate.ToJSON();
		try
		{
			FileUtil::WriteTextToFile(FileUtil::ToPath(file), JsonUtil::WriteCondensed(slateJSON));
			WALLET_INFO_F("Slate file saved to: {}", file);

			Json::Value result;
			result["status"] = "RECEIVED";
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

	IWalletManagerPtr m_pWalletManager;
};