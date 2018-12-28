#pragma once

#include <Core/TransactionInput.h>
#include <Core/TransactionOutput.h>

class CutThroughUtil
{
public:
	static void CutThrough(std::vector<TransactionInput>& inputs, std::vector<TransactionOutput>& outputs);
};