#pragma once

#include "../../Node/NodeContext.h"
#include <Core/Models/Transaction.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <cassert>

class SubmitTxMethod : public RPCMethod
{
public:
	SubmitTxMethod(std::shared_ptr<NodeContext> pNodeContext)
		: m_pNodeContext(pNodeContext)
	{
		assert(pNodeContext != nullptr);
	}

	virtual RPC::Response Handle(const RPC::Request& request) const override final
	{
		if (!request.GetParams().has_value())
		{
			return request.BuildError(RPC::ErrorCode::INVALID_PARAMS, "Params missing");
		}

		Json::Value txJSON = JsonUtil::GetRequiredField(*request.GetParams(), "tx");
		TransactionPtr pTransaction = std::make_shared<Transaction>(Transaction::FromJSON(txJSON));

		EBlockChainStatus status = m_pNodeContext->m_pBlockChainServer->AddTransaction(pTransaction, EPoolType::JOINPOOL);
		if (status == EBlockChainStatus::SUCCESS)
		{
			Json::Value response;
			response["STATUS"] = "SUCCESS";
			return request.BuildResult(response);
		}
		else
		{
			Json::Value error;
			error["STATUS"] = "ERROR"; // TODO: Determine status
			return request.BuildError(RPC::ErrorCode::INVALID_REQUEST, "Error occurred", error);
		}
	}

	bool ContainsSecrets() const noexcept final { return true; }

private:
	std::shared_ptr<NodeContext> m_pNodeContext;
};