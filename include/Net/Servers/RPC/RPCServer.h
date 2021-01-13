#pragma once

#include <Net/Servers/Server.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <Net/Clients/RPC/RPC.h>
#include <Core/Exceptions/APIException.h>
#include <unordered_map>
#include <cassert>

#define RPC_LOG_INFO_F(logFile, message, ...) LoggerAPI::LogInfo(logFile, __func__, __LINE__, StringUtil::Format(message, __VA_ARGS__))
#define RPC_LOG_ERROR_F(logFile, message, ...) LoggerAPI::LogError(logFile, __func__, __LINE__, StringUtil::Format(message, __VA_ARGS__))

class RPCServer
{
public:
	using Ptr = std::shared_ptr<RPCServer>;

	static std::shared_ptr<RPCServer> Create(
		const EServerType type,
		const std::optional<uint16_t>& port,
		const std::string& uri,
		const LoggerAPI::LogFile& logFile)
	{
		ServerPtr pServer = Server::Create(type, port);
		return RPCServer::Create(pServer, uri, logFile);
	}

	static RPCServer::Ptr Create(
		const ServerPtr& pServer,
		const std::string& uri,
		const LoggerAPI::LogFile& logFile)
	{
		auto pRPCServer = std::shared_ptr<RPCServer>(new RPCServer(pServer, logFile));

		WALLET_INFO_F("Adding RPC listener:  {}", uri);

		pServer->AddListener(uri, APIHandler, pRPCServer.get());

		WALLET_INFO_F("Added RPC listener:  {}", uri);

		return pRPCServer;
	}

	uint16_t GetPortNumber() const noexcept { return m_pServer->GetPortNumber(); }

	void AddMethod(const std::string& method, const std::shared_ptr<RPCMethod>& pMethod) noexcept
	{
		m_methods[method] = pMethod;
	}

	const ServerPtr& GetServer() const { return m_pServer; }

private:
	RPCServer(const ServerPtr& pServer, const LoggerAPI::LogFile& logFile)
		: m_pServer(pServer), m_logFile(logFile) { }

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
					if (!iter->second->ContainsSecrets())
					{
						RPC_LOG_INFO_F(instance.m_logFile, "REQUEST: {}", request.ToString());
					}
					else
					{
						RPC_LOG_INFO_F(
							instance.m_logFile,
							"Request made with method: {}",
							method
						);
					}
					
					auto response = iter->second->Handle(request);

					if (!iter->second->ContainsSecrets())
					{
						RPC_LOG_INFO_F(instance.m_logFile, "REPLY: {}", response.ToString());
					}
					else if (response.GetError().has_value())
					{
						RPC_LOG_INFO_F(
							instance.m_logFile,
							"Method: {}, Error: {}",
							method,
							response.GetError().value().ToString()
						);
					}

					return response;
				}
				else
				{
					RPC_LOG_ERROR_F(instance.m_logFile, "Method not supported: {}", method);
					return request.BuildError(
						RPC::ErrorCode::METHOD_NOT_FOUND,
						"Method not supported"
					);
				}
			}
			catch (const DeserializationException& e)
			{
				RPC_LOG_ERROR_F(instance.m_logFile, "Failed to deserialize request: {}", e.what());
				return request.BuildError(
					RPC::ErrorCode::INVALID_PARAMS,
					StringUtil::Format("Failed to deserialize request: {}", e.what())
				);
			}
		}
		catch (const RPCException& e)
		{
			RPC_LOG_ERROR_F(instance.m_logFile, "RPCException occurred: {}", e.what());
			return RPC::Response::BuildError(
				e.GetId().value_or(Json::nullValue),
				RPC::ErrorCode::INVALID_REQUEST,
				e.what()
			);
		}
		catch (const APIException& e)
		{
			RPC_LOG_ERROR_F(instance.m_logFile, "APIException occurred: {}", e.what());
			return RPC::Response::BuildError(
				id,
				e.GetErrorCode(),
				e.GetMsg()
			);
		}
		catch (const std::exception& e)
		{
			RPC_LOG_ERROR_F(instance.m_logFile, "Exception occurred: {}", e.what());
			return RPC::Response::BuildError(
				id,
				RPC::ErrorCode::INVALID_REQUEST,
				e.what()
			);
		}
	}

	ServerPtr m_pServer;
	std::unordered_map<std::string, std::shared_ptr<RPCMethod>> m_methods;
	LoggerAPI::LogFile m_logFile;
};

typedef std::shared_ptr<RPCServer> RPCServerPtr;