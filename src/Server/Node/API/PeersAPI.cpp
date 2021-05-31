#include "PeersAPI.h"
#include "../NodeContext.h"

#include <Net/Util/HTTPUtil.h>
#include <Common/Util/StringUtil.h>
#include <json/json.h>

//
// Handles requests to retrieve all known peers.
//
// APIs:
// GET /v1/peers/all
//
int PeersAPI::GetAllPeers_Handler(struct mg_connection* conn, void* pNodeContext)
{
	NodeContext* pServer = (NodeContext*)pNodeContext;

	try
	{
		const std::vector<PeerConstPtr> peers = pServer->m_pP2PServer->GetAllPeers();

		Json::Value json;
		for (PeerConstPtr peer : peers)
		{
			json.append(peer->ToJSON());
		}

		return HTTPUtil::BuildSuccessResponse(conn, json.toStyledString());
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception thrown: {}", e.what());
		return HTTPUtil::BuildInternalErrorResponse(conn, "Unknown error occurred");
	}
}

//
// Handles requests to retrieve all connected peers.
//
// APIs:
// GET /v1/peers/connected
//
int PeersAPI::GetConnectedPeers_Handler(struct mg_connection* conn, void* pNodeContext)
{
	NodeContext* pServer = (NodeContext*)pNodeContext;

	try
	{
		const std::vector<ConnectedPeer> connectedPeers = pServer->m_pP2PServer->GetConnectedPeers();

		Json::Value json;
		for (const ConnectedPeer& connectedPeer : connectedPeers)
		{
			json.append(connectedPeer.ToJSON());
		}

		return HTTPUtil::BuildSuccessResponse(conn, json.toStyledString());
	}
	catch (std::exception & e)
	{
		LOG_ERROR_F("Exception thrown: {}", e.what());
		return HTTPUtil::BuildInternalErrorResponse(conn, "Unknown error occurred");
	}
}

//
// Handles requests to retrieve, ban, or unban a peer.
//
// APIs:
// GET /v1/peers/a.b.c.d
// POST /v1/peers/a.b.c.d/ban
// POST /v1/peers/a.b.c.d/unban
//
int PeersAPI::Peer_Handler(struct mg_connection* conn, void* pNodeContext)
{
	NodeContext* pServer = (NodeContext*)pNodeContext;
	const std::string requestedPeer = HTTPUtil::GetURIParam(conn, "/v1/peers/");
	if (requestedPeer.empty())
	{
		return GetAllPeers_Handler(conn, pNodeContext);
	}

	try
	{
		const HTTP::EHTTPMethod method = HTTPUtil::GetHTTPMethod(conn);

		const std::string ipAddressStr = ParseIPAddress(requestedPeer);
		IPAddress ipAddress = IPAddress::Parse(ipAddressStr);

		const std::string commandStr = ParseCommand(requestedPeer);
		if (commandStr == "ban" && method == HTTP::EHTTPMethod::POST)
		{
			pServer->m_pP2PServer->BanPeer(ipAddress, EBanReason::ManualBan);
			return HTTPUtil::BuildSuccessResponse(conn, "");
		}
		else if (commandStr == "unban" && method == HTTP::EHTTPMethod::POST)
		{
			pServer->m_pP2PServer->UnbanPeer(ipAddress);

			return HTTPUtil::BuildSuccessResponse(conn, "");
		}
		else if (commandStr == "" && method == HTTP::EHTTPMethod::GET)
		{
			std::optional<PeerConstPtr> peerOpt = pServer->m_pP2PServer->GetPeer(ipAddress);
			if (peerOpt.has_value())
			{
				return HTTPUtil::BuildSuccessResponse(
					conn,
					Json::Value().append(peerOpt.value()->ToJSON()).toStyledString()
				);
			}
		}
		else
		{
			return HTTPUtil::BuildBadRequestResponse(conn, "Invalid command.");
		}
	}
	catch (const DeserializationException&)
	{
		return HTTPUtil::BuildBadRequestResponse(conn, "Invalid IP address.");
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception thrown: {}", e.what());
	}

	return HTTPUtil::BuildNotFoundResponse(conn, "Peer not found.");
}

std::string PeersAPI::ParseIPAddress(const std::string& request)
{
	const std::size_t colonPos = request.find(":");
	if (colonPos != std::string::npos)
	{
		return request.substr(0, colonPos);
	}

	const std::size_t slashPos = request.find("/");
	if (slashPos != std::string::npos)
	{
		return request.substr(0, slashPos);
	}

	return request;
}

std::string PeersAPI::ParseCommand(const std::string& request)
{
	const std::size_t slashPos = request.find("/");
	if (slashPos != std::string::npos)
	{
		return request.substr(slashPos + 1);
	}

	return "";
}