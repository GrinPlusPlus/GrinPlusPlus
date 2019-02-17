#pragma once

#include <Core/Models/BlockHeader.h>
#include <Config/Config.h>

// Forward Declarations
class IHeaderMMR;

class BlockHeaderValidator
{
public:
	BlockHeaderValidator(const Config& config, const IHeaderMMR& headerMMR);

	bool IsValidHeader(const BlockHeader& header, const BlockHeader& previousHeader) const;

	const Config& m_config;
	const IHeaderMMR& m_headerMMR;
};