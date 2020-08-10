#pragma once

#include <Core/Models/DTOs/BlockIdentifier.h>
#include <Core/Models/DTOs/OutputDTO.h>

class BlockWithOutputs
{
public:
	BlockWithOutputs(BlockIdentifier&& blockIdentifier, std::vector<OutputDTO>&& outputs)
		: m_blockIdentifier(std::move(blockIdentifier)), m_outputs(std::move(outputs)) { }

	const BlockIdentifier& GetBlockIdentifier() const noexcept { return m_blockIdentifier; }
	const std::vector<OutputDTO>& GetOutputs() const noexcept { return m_outputs; }

private:
	BlockIdentifier m_blockIdentifier;
	std::vector<OutputDTO> m_outputs;
};