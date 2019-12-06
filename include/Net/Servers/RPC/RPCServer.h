#pragma once

#include <Net/Servers/Server.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <Net/Clients/RPC/RPC.h>
#include <unordered_map>
#include <cassert>

class RPCServer
{
public:
	static std::shared_ptr<RPCServer> Create(const EServerType type, const std::optional<uint16_t>& port, const std::string& uri)
	{
		ServerPtr pServer = Server::Create(type, port);
		auto pRPCServer = std::shared_ptr<RPCServer>(new RPCServer(pServer));

		pServer->AddListener(uri, APIHandler, pRPCServer.get());

		return pRPCServer;
	}

	uint16_t GetPortNumber() const noexcept { return m_pServer->GetPortNumber(); }

	void AddMethod(const std::string& method, std::shared_ptr<RPCMethod> pMethod) noexcept
	{
		m_methods[method] = pMethod;
	}

private:
	RPCServer(ServerPtr pServer) : m_pServer(pServer) { }

	static int APIHandler(mg_connection* pConnection, void* pCbContext)
	{
		RPCServer* pInstance = static_cast<RPCServer*>(pCbContext);
		assert(pInstance != nullptr);

		Json::Value responseJSON = Handle(pConnection, *pInstance).ToJSON();

		return HTTPUtil::BuildSuccessResponseJSON(pConnection, responseJSON);
	}

	static RPC::Response Handle(mg_connection* pConnection, RPCServer& instance)
	{
		Json::Value id(Json::nullValue);
		try
		{
			RPC::Request request = RPC::Request::Parse(pConnection);
			id = request.GetId();
			try
			{
				const std::string& method = request.GetMethod();
				auto iter = instance.m_methods.find(method);
				if (iter != instance.m_methods.end())
				{
					return iter->second->Handle(request);
				}
				else
				{
					return request.BuildError(RPC::ErrorCode::METHOD_NOT_FOUND, "Method not supported");
				}
			}
			catch (DeserializationException&)
			{
				return request.BuildError(RPC::ErrorCode::INVALID_PARAMS, "Failed to deserialize request");
			}
		}
		catch (const RPCException& e)
		{
			return RPC::Response::BuildError(e.GetId().value_or(Json::nullValue), RPC::ErrorCode::INVALID_REQUEST, e.what());
		}
		catch (const std::exception& e)
		{
			return RPC::Response::BuildError(id, RPC::ErrorCode::INVALID_REQUEST, e.what());
		}
	}

	ServerPtr m_pServer;
	std::unordered_map<std::string, std::shared_ptr<RPCMethod>> m_methods;
};

typedef std::shared_ptr<RPCServer> RPCServerPtr;