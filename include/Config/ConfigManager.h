#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <ImportExport.h>
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
	static Config LoadConfig();
};