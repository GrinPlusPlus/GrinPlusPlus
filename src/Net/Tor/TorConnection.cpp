#include <Net/Tor/TorConnection.h>
#include <Core/Util/JsonUtil.h>
#include <Net/RPC.h>

TorConnection::TorConnection(const TorAddress& address, Socks5Connection&& connection)
	: m_address(address), m_socksConnection(std::move(connection))
{

}

bool TorConnection::CheckVersion()
{
	try
	{
		RPC::Request checkVersionRequest = RPC::Request::BuildRequest("check_version");
		std::unique_ptr<RPC::Response> pResponse = m_socksConnection.Send(checkVersionRequest);
		if (pResponse == nullptr || pResponse->GetError().has_value())
		{
			return false;
		}

		const Json::Value responseJSON = pResponse->GetResult().value();
		const Json::Value okJSON = JsonUtil::GetRequiredField(responseJSON, "Ok");
		const uint64_t apiVersion = JsonUtil::GetRequiredUInt64(responseJSON, "foreign_api_version");
		if (apiVersion != 2)
		{
			return false;
		}

		const Json::Value slateVersionsJSON = JsonUtil::GetRequiredField(okJSON, "supported_slate_versions");
		for (const Json::Value& slateVersion : slateVersionsJSON)
		{
			if (std::string(slateVersion.asCString()) == "V2")
			{
				return true;
			}
		}

		return true;
	}
	catch (const DeserializationException& e)
	{
		return false;
	}
}