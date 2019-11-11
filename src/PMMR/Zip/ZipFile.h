#pragma once

#include "minizip/unzip.h"

#ifdef _WIN32
	#include "minizip/iowin32.h"
	#define USEWIN32IOAPI
#endif

#include <string>
#include <vector>
#include <memory>

/*
 * Thin wrapper on minizip's unzip library to give simple object oriented access to a zip file
 */
class ZipFile
{
public:
	static std::shared_ptr<ZipFile> Load(const std::string& zipFilePath);
	~ZipFile();

	void ExtractFile(const std::string& path, const std::string& destination) const;
	std::vector<std::string> ListFiles() const;

private:
	ZipFile(const std::string& zipFilePath, const unzFile& file);

	std::string m_zipFilePath;
	unzFile m_unzFile;
};