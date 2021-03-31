#pragma once

#include <Core/Models/BlockHeader.h>
#include <Database/BlockDb.h>
#include <PMMR/HeaderMMR.h>

class BlockHeaderValidator
{
public:
	BlockHeaderValidator(const IBlockDB::CPtr& pBlockDB, const IHeaderMMR::CPtr& pHeaderMMR)
		: m_pBlockDB(pBlockDB), m_pHeaderMMR(pHeaderMMR) { }

	/// <summary>
	/// Ensures the height, version, timestamp, PoW, and MMR root of the header is valid.
	/// </summary>
	/// <param name="header">The header to validate.</param>
	/// <param name="prev_header">The previous header.</param>
	/// <throws>BadDataException if the header is invalid.</throws>
	void Validate(const BlockHeader& header, const BlockHeader& prev_header) const;

private:
	bool IsPoWValid(const BlockHeader& header, const BlockHeader& prev_header) const;

	IBlockDB::CPtr m_pBlockDB;
	IHeaderMMR::CPtr m_pHeaderMMR;
};