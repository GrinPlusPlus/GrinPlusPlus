#include "PeersAPI.h"
#include "../../JSONFactory.h"
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

	const std::vector<PeerConstPtr> peers = pServer->m_pP2PServer->GetAllPeers();

	Json::Value rootNode;

	for (PeerConstPtr peer : peers)
	{
		rootNode.append(JSONFactory::BuildPeerJSON(*peer));
	}

	return HTTPUtil::BuildSuccessResponse(conn, rootNode.toStyledString());
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

	return HTTPUtil::BuildSuccessResponse(conn, rootNode.toStyledString());
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
		IPAddress ipAddress = IPAddress::FromString(ipAddressStr);

		const std::string commandStr = ParseCommand(requestedPeer);
		if (commandStr == "ban" && method == HTTP::EHTTPMethod::POST)
		{
			const bool banned = pServer->m_pP2PServer->BanPeer(ipAddress, EBanReason::ManualBan);
			if (banned)
			{
				return HTTPUtil::BuildSuccessResponse(conn, "");
			}
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
				Json::Value rootNode;
				rootNode.append(JSONFactory::BuildPeerJSON(*peerOpt.value()));

				return HTTPUtil::BuildSuccessResponse(conn, rootNode.toStyledString());
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