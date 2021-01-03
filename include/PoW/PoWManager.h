#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Core/Config.h>
#include <Core/Models/BlockHeader.h>
#include <Database/BlockDb.h>

#define POW_API

//
// Entrypoint for the PoW module.
//
class POW_API PoWManager
{
public:
	PoWManager(const Config& config, std::shared_ptr<const IBlockDB> pBlockDB);
	~PoWManager() = default;

	//
	// Validates the difficulty, algo, etc of the header's proof of work.
	// Returns true if the PoW is valid.
	//
	bool IsPoWValid(
		const BlockHeader& header,
		const BlockHeader& previousHeader
	) const;

private:
	const Config& m_config;
	std::shared_ptr<const IBlockDB> m_pBlockDB;
};