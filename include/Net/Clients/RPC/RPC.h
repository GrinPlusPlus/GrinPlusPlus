#pragma once

#include <Core/Util/JsonUtil.h>
#include <Core/Traits/Jsonable.h>
#include <Net/Util/HTTPUtil.h>
#include <Net/Clients/RPC/RPCException.h>
#include <atomic>

namespace RPC
{

static std::atomic_int ID_COUNTER = 1;

enum ErrorCode
{
	INVALID_JSON = -32700,		// Parse error
	INVALID_REQUEST = -32600,	// Invalid Request
	METHOD_NOT_FOUND = -32601,	// Method not found
	INVALID_PARAMS = -32602,	// Invalid params
	INTERNAL_ERROR = -32603		// Internal error
};

class Error
{
public:
	Error(const int code, const std::string& message, const std::optional<Json::Value>& data = std::nullopt)
		: m_code(code), m_message(message), m_data(data)
	{

	}

	inline int GetCode() const { return m_code; }
	inline const std::string& GetMsg() const { return m_message; }
	inline const std::optional<Json::Value>& GetData() const { return m_data; }

	static Error Parse(const Json::Value& error)
	{
		int code = (int)JsonUtil::GetRequiredInt64(error, "code");
		std::string message = JsonUtil::GetRequiredString(error, "message");
		std::optional<Json::Value> data = JsonUtil::GetOptionalField(error, "data");

		return Error(code, message, data);
	}

	Json::Value ToJSON() const
	{
		Json::Value json;
		json["code"] = m_code;
		json["message"] = m_message;
		if (m_data.has_value())
		{
			json["data"] = m_data.value();
		}

		return json;
	}

	std::string ToString() const
	{
		return JsonUtil::WriteCondensed(ToJSON());
	}

private:
	int m_code;
	std::string m_message;
	std::optional<Json::Value> m_data;
};

class Response
{
public:
	Response(const Response& other) = default;

	static Response BuildResult(const Json::Value& id, const Json::Value& result)
	{
		return Response(Json::Value(id), std::make_optional(result), std::nullopt);
	}

	static Response BuildError(const Json::Value& id, const int code, const std::string& message, const std::optional<Json::Value>& data = std::nullopt)
	{
		Error error(code, message, data.value_or(Json::nullValue));
		return Response(Json::Value(id), std::nullopt, std::make_optional(std::move(error)));
	}

	static Response Parse(const std::string& jsonStr)
	{
		try
		{
			Json::Value json;
			if (!JsonUtil::Parse(jsonStr, json))
			{
				throw RPC_EXCEPTION("invalid json", std::nullopt);
			}

			// Parse id
			std::optional<Json::Value> idOpt = JsonUtil::GetOptionalField(json, "id");
			if (!idOpt.has_value())
			{
				throw RPC_EXCEPTION("id is missing", std::nullopt);
			}

			Json::Value id = idOpt.value();

			// Parse jsonrpc
			std::optional<Json::Value> jsonrpcOpt = JsonUtil::GetOptionalField(json, "jsonrpc");
			if (!jsonrpcOpt.has_value())
			{
				throw RPC_EXCEPTION("jsonrpc is missing", id);
			}
			else if (!jsonrpcOpt.value().isString())
			{
				throw RPC_EXCEPTION("jsonrpc must be a string", id);
			}

			std::string jsonrpc(jsonrpcOpt.value().asString());
			if (jsonrpc != "2.0")
			{
				throw RPC_EXCEPTION("invalid jsonrpc value: " + jsonrpc, id);
			}

			// Parse params
			std::optional<Json::Value> resultOpt = JsonUtil::GetOptionalField(json, "result");
			std::optional<Json::Value> errorJsonOpt = JsonUtil::GetOptionalField(json, "error");
			std::optional<Error> errorOpt = std::nullopt;
			if (errorJsonOpt.has_value())
			{
				errorOpt = std::make_optional(Error::Parse(errorJsonOpt.value()));
			}

			return Response(std::move(id), std::move(resultOpt), std::move(errorOpt));
		}
		catch (const RPCException& e)
		{
			throw e;
		}
		catch (const std::exception& e)
		{
			throw RPC_EXCEPTION(e.what(), std::nullopt);
		}
	}

	const Json::Value& GetId() const noexcept { return m_id; }
	const std::optional<Json::Value>& GetResult() const noexcept { return m_resultOpt; }
	const std::optional<Error>& GetError() const noexcept { return m_errorOpt; }

	Json::Value ToJSON() const
	{
		Json::Value json;
		json["jsonrpc"] = "2.0";
		json["id"] = m_id;

		if (m_resultOpt.has_value())
		{
			json["result"] = m_resultOpt.value();
		}
		else
		{
			json["error"] = m_errorOpt.value().ToJSON();
		}

		return json;
	}

	std::string ToString() const
	{
		return JsonUtil::WriteCondensed(ToJSON());
	}

private:
	Response(Json::Value&& id, std::optional<Json::Value>&& resultOpt, std::optional<Error>&& errorOpt)
		: m_id(std::move(id)), m_resultOpt(std::move(resultOpt)), m_errorOpt(std::move(errorOpt))
	{

	}

	Json::Value m_id;
	std::optional<Json::Value> m_resultOpt;
	std::optional<Error> m_errorOpt;
};

class Request
{
public:
	static Request Parse(mg_connection* pConnection)
	{
		try
		{
			std::optional<Json::Value> jsonOpt = HTTPUtil::GetRequestBody(pConnection);
			if (!jsonOpt.has_value())
			{
				throw RPC_EXCEPTION("json missing or invalid", std::nullopt);
			}

			const Json::Value& json = jsonOpt.value();

			// Parse id
			std::optional<Json::Value> idOpt = JsonUtil::GetOptionalField(json, "id");
			if (!idOpt.has_value())
			{
				throw RPC_EXCEPTION("id is missing", std::nullopt);
			}

			Json::Value id = idOpt.value();

			// Parse jsonrpc
			std::optional<Json::Value> jsonrpcOpt = JsonUtil::GetOptionalField(json, "jsonrpc");
			if (!jsonrpcOpt.has_value())
			{
				throw RPC_EXCEPTION("jsonrpc is missing", id);
			}
			else if (!jsonrpcOpt.value().isString())
			{
				throw RPC_EXCEPTION("jsonrpc must be a string", id);
			}

			std::string jsonrpc(jsonrpcOpt.value().asString());
			if (jsonrpc != "2.0")
			{
				throw RPC_EXCEPTION("invalid jsonrpc value: " + jsonrpc, id);
			}

			// Parse method
			std::optional<Json::Value> methodOpt = JsonUtil::GetOptionalField(json, "method");
			if (!methodOpt.has_value())
			{
				throw RPC_EXCEPTION("method is missing", id);
			}
			else if (!methodOpt.value().isString())
			{
				throw RPC_EXCEPTION("method must be a string", id);
			}

			const std::string method(methodOpt.value().asString());
			if (method.empty())
			{
				throw RPC_EXCEPTION("method must not be empty", id);
			}

			// Parse params
			std::optional<Json::Value> paramsOpt = JsonUtil::GetOptionalField(json, "params");

			return Request(std::move(id), method, std::move(paramsOpt));
		}
		catch (const RPCException& e)
		{
			throw e;
		}
		catch (const std::exception& e)
		{
			throw RPC_EXCEPTION(e.what(), std::nullopt);
		}
	}

	static Request BuildRequest(const std::string& method)
	{
		Json::Value id = ID_COUNTER++;
		return Request(std::move(id), method, std::nullopt);
	}

	static Request BuildRequest(const std::string& method, const Json::Value& params)
	{
		Json::Value id = ID_COUNTER++;
		return Request(std::move(id), method, std::make_optional(params));
	}

	Response BuildResult(const Traits::IJsonable& result) const
	{
		return Response::BuildResult(m_id, result.ToJSON());
	}

	Response BuildResult(const Json::Value& result) const
	{
		return Response::BuildResult(m_id, result);
	}

	Response BuildError(const int code, const std::string& message, const std::optional<Json::Value>& data = std::nullopt) const
	{
		return Response::BuildError(m_id, code, message, data);
	}

	// TODO: Use enum and lookup id, type, and message
	Response BuildError(const std::string& type, const std::string& message) const
	{
		Json::Value errorData;
		errorData["type"] = type;
		return Response::BuildError(m_id, -100, message, errorData);
	}

	Response BuildError(const Error& error) const
	{
		return Response::BuildError(m_id, error.GetCode(), error.GetMsg(), error.GetData());
	}

	Json::Value ToJSON() const
	{
		Json::Value json;
		json["id"] = m_id;
		json["jsonrpc"] = "2.0";
		json["method"] = m_method;

		if (m_paramsOpt.has_value())
		{
			json["params"] = m_paramsOpt.value();
		}

		return json;
	}

	std::string ToString() const
	{
		return JsonUtil::WriteCondensed(ToJSON());
	}

	const Json::Value& GetId() const { return m_id; }
	const std::string& GetMethod() const { return m_method; }
	const std::optional<Json::Value>& GetParams() const { return m_paramsOpt; }

private:
	Request(Json::Value&& id, const std::string& method, std::optional<Json::Value>&& paramsOpt)
		: m_id(std::move(id)), m_method(method), m_paramsOpt(std::move(paramsOpt))
	{

	}

	Json::Value m_id;
	std::string m_method;
	std::optional<Json::Value> m_paramsOpt;
};

}