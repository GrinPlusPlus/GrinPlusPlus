#pragma once

#include <Core/Models/TransactionInput.h>
#include <Core/Models/TransactionOutput.h>
#include <Common/Util/FunctionalUtil.h>
#include <set>

class TransactionUtil
{
public:
	static void PerformCutThrough(std::vector<TransactionInput>& inputs, std::vector<TransactionOutput>& outputs)
	{
		std::set<Commitment> inputCommitments;
		for (const TransactionInput& input : inputs)
		{
			inputCommitments.insert(input.GetCommitment());
		}

		std::set<Commitment> outputCommitments;
		for (const TransactionOutput& output : outputs)
		{
			outputCommitments.insert(output.GetCommitment());
		}

		auto inputIter = inputs.begin();
		while (inputIter != inputs.end())
		{
			if (outputCommitments.find(inputIter->GetCommitment()) != outputCommitments.end())
			{
				inputIter = inputs.erase(inputIter);
			}
			else
			{
				++inputIter;
			}
		}
		auto outputIter = outputs.begin();
		while (outputIter != outputs.end())
		{
			if (inputCommitments.find(outputIter->GetCommitment()) != inputCommitments.end())
			{
				outputIter = outputs.erase(outputIter);
			}
			else
			{
				++outputIter;
			}
		}
	}
};