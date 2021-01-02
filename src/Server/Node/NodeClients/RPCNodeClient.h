#pragma once

#include <Wallet/NodeClient.h>
#include <Net/Connections/HttpConnection.h>
#include <Core/Exceptions/UnimplementedException.h>
#include <Common/Macros.h>
#include <memory>

// TODO: Implement caching & retry policies
class RPCNodeClient : public INodeClient
{
	enum class ENodeType
	{
		GRINPP,
		RUST
	};

public:
	RPCNodeClient(const ENodeType type, HttpConnection::UPtr&& pConnection)
		: m_type(type), m_pConnection(std::move(pConnection)) { }

	static INodeClientPtr Create(const std::string& host, const uint16_t port)
	{
		auto pConnection = HttpConnection::Connect(host, port);
		auto response = pConnection->Invoke("get_version");
		if (!response.GetResult().has_value()) {
			throw std::exception();
		}

		auto ok = JsonUtil::GetRequiredField(response.GetResult().value(), "Ok");
		auto version = JsonUtil::GetRequiredString(ok, "node_version");

		ENodeType type = StringUtil::StartsWith(version, "Grin++") ? ENodeType::GRINPP : ENodeType::RUST;

		return std::make_shared<RPCNodeClient>(type, std::move(pConnection));
	}

	//
	// Returns the current confirmed chain height.
	//
	uint64_t GetChainHeight() const final
	{
		return JsonUtil::GetRequiredUInt64(Invoke("get_tip"), "height");
	}

	//
	// Returns the header of the confirmed block at the given height, if it exists.
	//
	BlockHeaderPtr GetBlockHeader(const uint64_t height) const final
	{
		Json::Value params = Json::Value(Json::arrayValue);
		params.append(Json::UInt64(height));
		params.append(Json::nullValue);
		params.append(Json::nullValue);

		return BlockHeader::FromJSON(Invoke("get_header", params));
	}

	//
	// Returns the location (block height and mmr index) of each requested output, if it is *unspent*.
	//
	std::map<Commitment, OutputLocation> GetOutputsByCommitment(const std::vector<Commitment>& commitments) const final
	{
		Json::Value commitsJson;
		for (const Commitment& commit : commitments)
		{
			commitsJson.append(commit.ToHex());
		}

		Json::Value params;
		params.append(commitsJson);
		params.append(Json::nullValue);
		params.append(Json::nullValue);
		params.append(false);
		params.append(false);

		auto response = Invoke("get_outputs");

		std::map<Commitment, OutputLocation> outputsByCommitment;
		for (const auto& output : response)
		{
			if (!JsonUtil::GetRequiredBool(output, "spent"))
			{
				Commitment commitment = JsonUtil::GetCommitment(output, "commit");
				outputsByCommitment.insert({ commitment, OutputLocation::FromJSON(output) });
			}
		}

		return outputsByCommitment;
	}

	//
	// Returns a vector containing block ids and their outputs for the given range.
	//
	std::vector<BlockWithOutputs> GetBlockOutputs(const uint64_t startHeight, const uint64_t maxHeight) const final
	{
		UNUSED(startHeight);
		UNUSED(maxHeight);

		throw UNIMPLEMENTED_EXCEPTION;
	}

	//
	// Returns a list of outputs starting at the given insertion index.
	//
	std::unique_ptr<OutputRange> GetOutputsByLeafIndex(const uint64_t startIndex, const uint64_t maxNumOutputs) const final
	{
		Json::Value params(Json::arrayValue);
		params.append(Json::UInt64(startIndex));
		params.append(Json::nullValue);
		params.append(Json::UInt64(maxNumOutputs));
		params.append(true);

		auto response = Invoke("get_unspent_outputs", params);
		return std::make_unique<OutputRange>(OutputRange::FromJSON(response));
	}

	//
	// Posts the transaction to the P2P Network.
	//
	bool PostTransaction(TransactionPtr pTransaction, const EPoolType poolType) final
	{
		Json::Value params(Json::arrayValue);
		params.append(pTransaction->ToJSON());
		params.append(poolType == EPoolType::MEMPOOL);

		auto response = m_pConnection->Invoke("push_transaction", params);

		return response.GetResult().has_value();
	}

private:
	Json::Value Invoke(const std::string& method, const Json::Value& params) const
	{
		auto response = m_pConnection->Invoke(method, params);
		if (!response.GetResult().has_value()) {
			throw std::exception();
		}

		return JsonUtil::GetRequiredField(response.GetResult().value(), "Ok");
	}

	Json::Value Invoke(const std::string& method) const
	{
		auto response = m_pConnection->Invoke(method);
		if (!response.GetResult().has_value()) {
			throw std::exception();
		}

		return JsonUtil::GetRequiredField(response.GetResult().value(), "Ok");
	}

	ENodeType m_type;
	HttpConnection::UPtr m_pConnection;
};