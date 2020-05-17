#pragma once

#include <Wallet/NodeClient.h>
#include <Net/Connections/HttpConnection.h>
#include <Common/Exceptions/UnimplementedException.h>
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
	static INodeClientPtr Create(const SocketAddress& address)
	{
		// TODO: Check if Grin++ Node or Grin/MW Node. If Grin++, use more efficient APIs
		ENodeType type = ENodeType::RUST;

		return std::shared_ptr<INodeClient>(new RPCNodeClient(type, HttpConnection::Connect(address)));
	}

	//
	// Returns the current confirmed chain height.
	//
	uint64_t GetChainHeight() const final
	{
		RPC::Response response = m_pConnection->Invoke("get_tip");
		if (!response.GetResult().has_value())
		{
			throw std::exception();
		}

		auto ok = JsonUtil::GetRequiredField(response.GetResult().value(), "Ok");

		return JsonUtil::GetRequiredUInt64(ok, "height");
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
		auto response = m_pConnection->Invoke("get_header", params);

		auto ok = JsonUtil::GetRequiredField(response.GetResult().value(), "Ok");
		return BlockHeader::FromJSON(ok);
	}

	//
	// Returns the location (block height and mmr index) of each requested output, if it is *unspent*.
	//
	std::map<Commitment, OutputLocation> GetOutputsByCommitment(const std::vector<Commitment>& commitments) const final
	{
		UNUSED(commitments);

		throw UNIMPLEMENTED_EXCEPTION;
	}

	//
	// Returns a vector containing block ids and their outputs for the given range.
	//
	std::vector<BlockWithOutputs> GetBlockOutputs(const uint64_t startHeight, const uint64_t maxHeight) const final
	{
		UNUSED(startHeight);
		UNUSED(maxHeight);

		/*
		get_outputs(
			&self,
			commits: Option<Vec<String>>,
			start_height: Option<u64>,
			end_height: Option<u64>,
			include_proof: Option<bool>,
			include_merkle_proof: Option<bool>
		)

		"method": "get_outputs",
		"params": [
		[
			"09bab2bdba2e6aed690b5eda11accc13c06723ca5965bb460c5f2383655989af3f",
			"08ecd94ae293863286e99d37f4685f07369bc084ba74d5c59c7f15359a75c84c03"
		],
		376150,
		376154,
		true,
		true
		],

			"result": {
		"Ok": [
		{
			"block_height": 374568,
			"commit": "09bab2bdba2e6aed690b5eda11accc13c06723ca5965bb460c5f2383655989af3f",
			"merkle_proof": null,
			"mmr_index": 4093403,
			"output_type": "Transaction",
			"proof": "e30aa961d6f89361a9a3c60f73e3551f50a3887212e524b5874ac50c1759bb95bc8e588d82dd51d84c7cbaa9abe79e0b8fe902bcfda17276c24d269fbf636aa2016c65a760a02e18338a33e83dec8e51fbfd953ee5b765d97ce39ba0850790d2104812a1d15d5eaa174de548144d3a7d413906d85e22f89065ef727910ee4c573494520c43e36e83dacee8096666aa4033b5e8322e72930c3f8476bb7be9aef0838a2ad6c28f4f5212708bf3e5954fc3971d66b7835383b96406fa65415b64ecd53a747f41d785c3e3615c18dfdbe39a0920fefcf6bc55fe65b4b215b1ad98c80fdafbef6f21ab60596f2d9a3e7bc45d750e807d5eb883dadde1625d4f20af9f1315b8bea08c97fad922afe2000c84c9eb5f96b2a24da7a637f95c1102ecfc1257e19bc4120082f5ee76448c90abd55108256f8341e0f4009cfc3906a598de465467ee1ee072bfd3384e1a0b9039192d1edc33092d7b09d1164c4fc4c378227a391600a8a5d5ba5fe36a2a4eabe0dbae270aefa5a5f2df810cda79211805206ad93ae08689e2675aad025db3499d43f1effc110dfb2f540ccd6eb972c02f98e8151535c099381c8aeb1ea8aad2cfdf952e6ab9d26e74a5611d943d02315e212eb06ce2cd20b4675e6f245e5302cdb8b31d46bb2e718b50ecfad2d440323826570447c2498376c8bad6e4ee97bde41c47f6a20eea406d758c53fb9e8542f114c1a277a6335ad97fdc542c6bbec756dc4a9085c319fe6f0c9e1bb043f01a43c12aa6f4dff8b1220e7f16bc56dee9ccb59fb7c3b7aa6bb33b41c33d8e4b03b6b9cb89491504210dd691b46ffe2862387339d2b62a9dc4c20d629e23eb8b06490c4999433c1b4626fb4d21517072bd8e82511c115ee47bf9a5e40f0a74177f5b573db2e277459877a01b172e026cbb3f76aaf0c61f244584f3a76804dea62175a80d777238",
			"proof_hash": "660d706330fc36f611c50d90cb965fddf750cc91f8891a58b5e39b83a5fc6b46",
			"spent": false
		},
		*/

		throw UNIMPLEMENTED_EXCEPTION;
	}

	//
	// Returns a list of outputs starting at the given insertion index.
	//
	std::unique_ptr<OutputRange> GetOutputsByLeafIndex(const uint64_t startIndex, const uint64_t maxNumOutputs) const final
	{
		UNUSED(startIndex);
		UNUSED(maxNumOutputs);

		throw UNIMPLEMENTED_EXCEPTION;
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
	RPCNodeClient(const ENodeType type, HttpConnection::UPtr&& pConnection)
		: m_type(type), m_pConnection(std::move(pConnection)) { }

	//std::vector<OutputDTO> GetOutputs(
	//	const std::vector<Commitment>& commitments,
	//	const uint64_t startHeight,
	//	const uint64_t endHeight)
	//{

	//}

	ENodeType m_type;
	HttpConnection::UPtr m_pConnection;
};