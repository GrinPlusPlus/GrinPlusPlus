#pragma once

#include <Core/Models/Display/BlockIdentifier.h>
#include <Core/Models/Display/OutputDisplayInfo.h>

class BlockWithOutputs
{
public:
	BlockWithOutputs(BlockIdentifier&& blockIdentifier, std::vector<OutputDisplayInfo>&& outputs)
		: m_blockIdentifier(std::move(blockIdentifier)), m_outputs(std::move(outputs))
	{

	}

	const BlockIdentifier& GetBlockIdentifier() const { return m_blockIdentifier; }
	const std::vector<OutputDisplayInfo>& GetOutputs() const { return m_outputs; }

private:
	BlockIdentifier m_blockIdentifier;
	std::vector<OutputDisplayInfo> m_outputs;
	// TODO: std::vector<uint64_t> m_prunedMMRIndices?
};