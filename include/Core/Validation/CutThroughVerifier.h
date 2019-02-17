#pragma once

#include <Core/Models/TransactionBody.h>
#include <set>

class CutThroughVerifier
{
public:
	static bool VerifyCutThrough(const TransactionBody& transactionBody)
	{
		return VerifyCutThrough(transactionBody.GetInputs(), transactionBody.GetOutputs());
	}

	static bool VerifyCutThrough(const std::vector<TransactionInput>& inputs, const std::vector<TransactionOutput>& outputs)
	{
		std::set<Commitment> commitments;
		for (auto output : outputs)
		{
			commitments.insert(output.GetCommitment());
		}

		for (auto input : inputs)
		{
			if (commitments.count(input.GetCommitment()) > 0)
			{
				return false;
			}
		}

		return true;
	}
};