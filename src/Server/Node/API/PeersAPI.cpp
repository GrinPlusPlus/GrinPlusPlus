#include "PeersAPI.h"
#include "../../RestUtil.h"
#include "../../JSONFactory.h"
#include "../NodeContext.h"

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

	const std::vector<Peer> peers = pServer->m_pP2PServer->GetAllPeers();

	Json::Value rootNode;

	for (const Peer& peer : peers)
	{
		rootNode.append(JSONFactory::BuildPeerJSON(peer));
	}

	return RestUtil::BuildSuccessResponse(conn, rootNode.toStyledString());
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

	const std::vector<ConnectedPeer> connectedPeers = pServer->m_pP2PServer->GetConnectedPeers();

	Json::Value rootNode;

	for (const ConnectedPeer& connectedPeer : connectedPeers)
	{
		rootNode.append(JSONFactory::BuildConnectedPeerJSON(connectedPeer));
	}

	return RestUtil::BuildSuccessResponse(conn, rootNode.toStyledString());
}

//
// Handles requests to retrieve, ban, or unban a peer.
//
// APIs:
// GET /v1/peers/a.b.c.d
// GET /v1/peers/a.b.c.d:p
// POST /v1/peers/a.b.c.d/ban
// POST /v1/peers/a.b.c.d:p/ban
// POST /v1/peers/a.b.c.d/unban
// POST /v1/peers/a.b.c.d:p/unban
//
int PeersAPI::Peer_Handler(struct mg_connection* conn, void* pNodeContext)
{
	NodeContext* pServer = (NodeContext*)pNodeContext;
	const std::string requestedPeer = RestUtil::GetURIParam(conn, "/v1/peers/");
	if (requestedPeer.empty())
	{
		return GetAllPeers_Handler(conn, pNodeContext);
	}

	try
	{
		const EHTTPMethod method = RestUtil::GetHTTPMethod(conn);

		const std::string ipAddressStr = ParseIPAddress(requestedPeer);
		IPAddress ipAddress = IPAddress::FromString(ipAddressStr);

		const std::string portStr = ParsePort(requestedPeer);
		const std::optional<uint16_t> portOpt = portStr.empty() ? std::nullopt : std::make_optional<uint16_t>((uint16_t)std::stoul(portStr));

		const std::string commandStr = ParseCommand(requestedPeer);
		if (commandStr == "ban" && method == EHTTPMethod::POST)
		{
			const bool banned = pServer->m_pP2PServer->BanPeer(ipAddress, portOpt, EBanReason::ManualBan);
			if (banned)
			{
				return RestUtil::BuildSuccessResponse(conn, "");
			}
		}
		else if (commandStr == "unban" && method == EHTTPMethod::POST)
		{
			const bool unbanned = pServer->m_pP2PServer->UnbanPeer(ipAddress, portOpt);
			if (unbanned)
			{
				return RestUtil::BuildSuccessResponse(conn, "");
			}
		}
		else if (commandStr == "" && method == EHTTPMethod::GET)
		{
			std::optional<Peer> peerOpt = pServer->m_pP2PServer->GetPeer(ipAddress, portOpt);
			if (peerOpt.has_value())
			{
				Json::Value rootNode;
				rootNode.append(JSONFactory::BuildPeerJSON(peerOpt.value()));

				return RestUtil::BuildSuccessResponse(conn, rootNode.toStyledString());
			}
		}
		else
		{
			return RestUtil::BuildBadRequestResponse(conn, "Invalid command.");
		}
	}
	catch (const DeserializationException&)
	{
		return RestUtil::BuildBadRequestResponse(conn, "Invalid IP address.");
	}

	return RestUtil::BuildNotFoundResponse(conn, "Peer not found.");
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

std::string PeersAPI::ParsePort(const std::string& request)
{
	const std::size_t colonPos = request.find(":");
	if (colonPos == std::string::npos)
	{
		return "";
	}

	const std::size_t slashPos = request.find("/");
	if (slashPos != std::string::npos)
	{
		return request.substr(colonPos + 1, slashPos - (colonPos + 1));
	}

	return request.substr(colonPos + 1);
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