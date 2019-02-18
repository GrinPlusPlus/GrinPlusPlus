#pragma once

#include <Core/Models/BlockHeader.h>
#include <Config/Config.h>

// Forward Declarations
class IHeaderMMR;
class IBlockDB;

class BlockHeaderValidator
{
public:
	BlockHeaderValidator(const Config& config, const IBlockDB& blockDB, const IHeaderMMR& headerMMR);

	bool IsValidHeader(const BlockHeader& header, const BlockHeader& previousHeader) const;

	const Config& m_config;
	const IBlockDB& m_blockDB;
	const IHeaderMMR& m_headerMMR;
};