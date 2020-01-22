#pragma once

#include "minizip/zip.h"

#include <filesystem.h>
#include <vector>
#include <string>

class Zipper
{
public:
	static void CreateZipFile(const fs::path& destination, const std::vector<fs::path>& paths);

private:
	static void AddDirectory(zipFile zf, const fs::path& sourceDir, const fs::path& destDir);
	static void AddFile(zipFile zf, const fs::path& sourceFile, const fs::path& destDir);
};