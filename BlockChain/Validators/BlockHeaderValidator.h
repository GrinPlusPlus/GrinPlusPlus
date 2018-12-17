#pragma once

#include <Core/BlockHeader.h>

// Forward Declarations
class IHeaderMMR;

class BlockHeaderValidator
{
public:
	BlockHeaderValidator(const IHeaderMMR& headerMMR);

	bool IsValidHeader(const BlockHeader& header, const BlockHeader& previousHeader) const;

	const IHeaderMMR& m_headerMMR;
};