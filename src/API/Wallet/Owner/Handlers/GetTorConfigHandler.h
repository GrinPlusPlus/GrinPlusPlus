#pragma once

#include <Core/Config.h>
#include <Core/Global.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <Wallet/Models/Slatepack/SlatepackAddress.h>
#include <Core/Util/JsonUtil.h>

class GetTorConfigHandler : public RPCMethod
{
public:
	GetTorConfigHandler(const TorProcess::Ptr& pTorProcess)
		: m_pTorProcess(pTorProcess) { }
	virtual ~GetTorConfigHandler() = default;
		
	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value()) {
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		const Json::Value params_json = request.GetParams().value();
		if (!params_json.isObject()) {
			return request.BuildError("INVALID_PARAMS", "Expected object parameter");
		}
		
		Config& config = Global::GetConfig();
		
		// check if params_json is and empty array
		if (params_json.isArray() && params_json.empty()) {
			Json::Value torrc_json;
			torrc_json["torrc"] = config.ReadTorrcFile();
			
			return request.BuildResult(torrc_json);
		}
		
		Json::Value result;
		result["status"] = "SUCCESS";
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	TorProcess::Ptr m_pTorProcess;
};