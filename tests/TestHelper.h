#pragma once

#include <Config/Config.h>
#include <Common/Util/FileUtil.h>

class TestHelper
{
public:
	static ConfigPtr GetTestConfig()
	{
		ConfigPtr pConfig = Config::Default(EEnvironmentType::AUTOMATED_TESTING);

		FileUtil::RemoveFile(pConfig->GetDataDirectory());

		return Config::Default(EEnvironmentType::AUTOMATED_TESTING);
	}
};