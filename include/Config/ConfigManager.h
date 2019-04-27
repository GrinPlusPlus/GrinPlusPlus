#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Common/ImportExport.h>
#include <Config/Config.h>

#ifdef MW_CONFIG
#define CONFIG_API EXPORT
#else
#define CONFIG_API IMPORT
#endif

class CONFIG_API ConfigManager
{
public:
	//
	// Loads and parses the config information from disk.
	//
	static Config LoadConfig(const EEnvironmentType environment);
};