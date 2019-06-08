#pragma once

#include "minizip/zip.h"

#include <filesystem.h>
#include <vector>
#include <string>

class Zipper
{
public:
	static bool CreateZipFile(const std::string& destination, const std::vector<std::string>& paths);

private:
	static bool AddDirectory(zipFile zf, const fs::path& sourceDir, const std::string& destDir);
	static bool AddFile(zipFile zf, const std::string& sourceFile, const std::string& destDir);
};