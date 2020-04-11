#pragma once

#include <Config/Config.h>
#include <Wallet/WalletManager.h>
#include <Net/Tor/TorManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <optional>

class RetryTorHandler : public RPCMethod
{
	class Response
	{
	public:
		Response() : m_status("FAILED"), m_torAddress(std::nullopt) { }

		void SetStatus(const std::string& status) noexcept { m_status = status; }
		void SetTorAddress(const std::string& address) noexcept { m_torAddress = std::make_optional(address); }

		RPC::Response BuildResponse(const RPC::Request& request) const
		{
			Json::Value result;
			result["status"] = m_status;

			if (m_torAddress.has_value())
			{
				result["tor_address"] = m_torAddress.value();
			}

			return request.BuildResult(result);
		}

	private:
		std::string m_status;
		std::optional<std::string> m_torAddress;
	};
public:
	RetryTorHandler(const Config& config, IWalletManagerPtr pWalletManager)
		: m_config(config), m_pWalletManager(pWalletManager) { }
	virtual ~RetryTorHandler() = default;

	virtual RPC::Response Handle(const RPC::Request& request) const override final
	{
		if (!request.GetParams().has_value())
		{
			throw DESERIALIZATION_EXCEPTION();
		}

		Response response;

		std::string tokenStr = JsonUtil::GetRequiredString(request.GetParams().value(), "session_token");
		SessionToken token = SessionToken::FromBase64(tokenStr);

		if (TorManager::GetInstance(m_config.GetTorConfig()).RetryInit())
		{
			auto listenerOpt = m_pWalletManager->AddTorListener(token, KeyChainPath::FromString("m/0/1/0"));
			if (listenerOpt.has_value())
			{
				response.SetStatus("SUCCESS");
				response.SetTorAddress(listenerOpt.value().ToString());
				return response.BuildResponse(request);
			}
		}

		return response.BuildResponse(request);
	}

private:
	const Config& m_config;
	IWalletManagerPtr m_pWalletManager;
};