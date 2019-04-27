#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Common/ImportExport.h>
#include <Config/Config.h>
#include <Core/Models/BlockHeader.h>
#include <Database/BlockDb.h>

#ifdef MW_POW
#define POW_API EXPORT
#else
#define POW_API IMPORT
#endif

class POW_API PoWManager
{
public:
	PoWManager(const Config& config, const IBlockDB& blockDB);
	~PoWManager() = default;

	bool IsPoWValid(const BlockHeader& header, const BlockHeader& previousHeader) const;

private:
	const Config& m_config;
	const IBlockDB& m_blockDB;
};