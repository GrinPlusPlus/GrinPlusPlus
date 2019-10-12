#pragma once

#include <Core/Util/JsonUtil.h>
#include <Net/HTTPUtil.h>
#include <Net/RPCException.h>

namespace RPC
{

static int ID_COUNTER = 1;

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
		return Request(std::move(id), method, std::make_optional<Json::Value>(params));
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

	inline const Json::Value& GetId() const { return m_id; }
	inline const std::string& GetMethod() const { return m_method; }
	inline const std::optional<Json::Value>& GetParams() const { return m_paramsOpt; }

private:
	Request(Json::Value&& id, const std::string& method, std::optional<Json::Value>&& paramsOpt)
		: m_id(std::move(id)), m_method(method), m_paramsOpt(std::move(paramsOpt))
	{

	}

	Json::Value m_id;
	std::string m_method;
	std::optional<Json::Value> m_paramsOpt;
};

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
	Error(const int code, const std::string& message, const Json::Value& data)
		: m_code(code), m_message(message), m_data(data)
	{

	}

	inline int GetCode() const { return m_code; }
	inline const std::string& GetMsg() const { return m_message; }
	inline const std::optional<Json::Value>& GetData() const { return m_data; }

	Json::Value ToJSON() const
	{
		Json::Value json;
		json["code"] = m_code;
		json["message"] = m_message;
		if (!m_data.isNull())
		{
			json["data"] = m_data;
		}

		return json;
	}

private:
	int m_code;
	std::string m_message;
	Json::Value m_data;
};

class Response
{
public:
	static Response BuildResult(const Json::Value& id, const Json::Value& result)
	{
		return Response(Json::Value(id), std::make_optional<Json::Value>(Json::Value(result)), std::nullopt);
	}

	static Response BuildError(const Json::Value& id, const int code, const std::string& message, const std::optional<Json::Value>& data)
	{
		Error error(code, message, data.value_or(Json::nullValue));
		return Response(Json::Value(id), std::nullopt, std::make_optional<Error>(std::move(error)));
	}

	inline const Json::Value& GetId() const { return m_id; }
	inline const std::optional<Json::Value>& GetResult() const { return m_resultOpt; }
	inline const std::optional<Error>& GetError() const { return m_errorOpt; }

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

private:
	Response(Json::Value&& id, std::optional<Json::Value>&& resultOpt, std::optional<Error>&& errorOpt)
		: m_id(std::move(id)), m_resultOpt(std::move(resultOpt)), m_errorOpt(std::move(errorOpt))
	{

	}

	Json::Value m_id;
	std::optional<Json::Value> m_resultOpt;
	std::optional<Error> m_errorOpt;
};

}