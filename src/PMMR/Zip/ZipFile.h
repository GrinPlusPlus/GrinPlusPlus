#pragma once

#include "minizip/unzip.h"

#ifdef _WIN32
	#include "minizip/iowin32.h"
	#define USEWIN32IOAPI
#endif

#include <filesystem.h>
#include <string>
#include <vector>
#include <memory>

/*
 * Thin wrapper on minizip's unzip library to give simple object oriented access to a zip file
 */
class ZipFile
{
public:
	static std::shared_ptr<ZipFile> Load(const fs::path& zipFilePath);
	~ZipFile();

	void ExtractFile(const std::string& path, const fs::path& destination) const;
	std::vector<std::string> ListFiles() const;

private:
	ZipFile(const fs::path& zipFilePath, const unzFile& file);

	fs::path m_zipFilePath;
	unzFile m_unzFile;
};