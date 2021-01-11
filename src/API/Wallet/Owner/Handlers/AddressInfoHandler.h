#pragma once

#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <Wallet/Models/Slatepack/SlatepackAddress.h>
#include <Core/Util/JsonUtil.h>

class AddressInfoHandler : public RPCMethod
{
public:
	AddressInfoHandler() = default;
	virtual ~AddressInfoHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value()) {
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		const Json::Value& json_params = request.GetParams().value();
		std::string grin1_address = JsonUtil::GetRequiredString(json_params, "slatepack_address");
		SlatepackAddress slatepack_address = SlatepackAddress::Parse(grin1_address);

		Json::Value result;
		result["slatepack_address"] = slatepack_address.ToString();
		result["tor_address"] = slatepack_address.ToTorAddress().ToString();
		result["onion_address"] = "http://" + slatepack_address.ToTorAddress().ToString() + ".onion";
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }
};