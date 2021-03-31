#pragma once

#include <Core/Models/BlockHeader.h>
#include <Database/BlockDb.h>

class PoWValidator
{
public:
	PoWValidator(const IBlockDB::CPtr& pBlockDB)
		: m_pBlockDB(pBlockDB) { }

	//
	// Validates the difficulty, algo, etc of the header's proof of work.
	// Returns true if the PoW is valid.
	//
	bool IsPoWValid(const BlockHeader& header, const BlockHeader& previousHeader) const;

private:
	uint64_t GetMaximumDifficulty(const BlockHeader& header) const;

	std::shared_ptr<const IBlockDB> m_pBlockDB;
};