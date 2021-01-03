#pragma once

#include <Core/Config.h>
#include <Common/Util/FileUtil.h>

class TestHelper
{
public:
	static ConfigPtr GetTestConfig()
	{
		ConfigPtr pConfig = Config::Default(Environment::AUTOMATED_TESTING);

		FileUtil::RemoveFile(pConfig->GetDataDirectory());

		return Config::Default(Environment::AUTOMATED_TESTING);
	}
};