#pragma once

#include <Core/Config.h>
#include <Wallet/WalletManager.h>
#include <Net/Tor/TorProcess.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
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
	RetryTorHandler(const TorProcess::Ptr& pTorProcess, IWalletManagerPtr pWalletManager)
		: m_pTorProcess(pTorProcess), m_pWalletManager(pWalletManager) { }
	~RetryTorHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		Response response;

		std::string tokenStr = JsonUtil::GetRequiredString(request.GetParams().value(), "session_token");
		SessionToken token = SessionToken::FromBase64(tokenStr);

		if (m_pTorProcess->RetryInit())
		{
			auto listenerOpt = m_pWalletManager->AddTorListener(
				token,
				KeyChainPath::FromString("m/0/1/0"),
				m_pTorProcess
			);
			if (listenerOpt.has_value())
			{
				response.SetStatus("SUCCESS");
				response.SetTorAddress(listenerOpt.value().ToString());
				return response.BuildResponse(request);
			}
		}

		return response.BuildResponse(request);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	TorProcess::Ptr m_pTorProcess;
	IWalletManagerPtr m_pWalletManager;
};