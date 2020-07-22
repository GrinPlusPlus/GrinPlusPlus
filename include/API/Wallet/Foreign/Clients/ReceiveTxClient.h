#pragma once

#include <Core/Util/JsonUtil.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Clients/RPC/RPCException.h>
#include <Net/Tor/TorAddress.h>
#include <Net/Tor/TorConnection.h>
#include <Net/Tor/TorProcess.h>
#include <Wallet/Models/Slate/Slate.h>

class ReceiveTxClient
{
public:
	static Slate Call(const TorProcess::Ptr& pTorProcess, const TorAddress& torAddress, const Slate& slate)
	{
		Json::Value params;
		params.append(slate.ToJSON());
		params.append(Json::nullValue); // Account path not currently supported
		params.append(Json::nullValue);
		RPC::Request receiveTxRequest = RPC::Request::BuildRequest("receive_tx", params);
		LOG_INFO_F("REQUEST: {}", receiveTxRequest.ToJSON().toStyledString());

		TorConnectionPtr pTorConnection = pTorProcess->Connect(torAddress);
		RPC::Response receiveTxResponse = pTorConnection->Invoke(receiveTxRequest, "/v2/foreign");

		LOG_INFO_F("RESPONSE: {}", receiveTxResponse.ToString());

		if (receiveTxResponse.GetError().has_value()) {
			RPC::Error rpc_error = receiveTxResponse.GetError().value();
			throw RPC_EXCEPTION(rpc_error.GetMsg(), std::nullopt);
		}

		Json::Value okJson = JsonUtil::GetRequiredField(receiveTxResponse.GetResult().value(), "Ok");

		return Slate::FromJSON(okJson);
	}
};