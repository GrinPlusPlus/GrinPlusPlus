#pragma once

#include "minizip/unzip.h"

#ifdef _WIN32
	#include "minizip/iowin32.h"
	#define USEWIN32IOAPI
#endif

#include <string>
#include <vector>

enum class EZipFileStatus
{
	SUCCESS,
	NOT_OPEN,
	NOT_FOUND,
	WRITE_FAILED
};

/*
 * Thin wrapper on minizip's unzip library to give simple object oriented access to a zip file
 */
class ZipFile
{
private:
	std::string m_zipFilePath;
	unzFile m_unzFile;

public:
	ZipFile(const std::string& zipFilePath);

	EZipFileStatus Open();
	void Close();

	EZipFileStatus ExtractFile(const std::string& path, const std::string& destination) const;
	EZipFileStatus ListFiles(std::vector<std::string>& files) const;
	//void CheckFile(const std::string& path) const;
};