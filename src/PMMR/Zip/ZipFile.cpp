#include "ZipFile.h"

#include <Core/Exceptions/FileException.h>
#include <Infrastructure/Logger.h>
#include <fstream>

ZipFile::ZipFile(const std::string& zipFilePath, const unzFile& file)
	: m_zipFilePath(zipFilePath), m_unzFile(file)
{

}

ZipFile::~ZipFile()
{
	unzClose(m_unzFile);
}

std::shared_ptr<ZipFile> ZipFile::Load(const std::string& zipFilePath)
{
	unzFile unzFile = unzOpen(zipFilePath.c_str());
	if (unzFile == NULL)
	{
		LOG_ERROR_F("Zip file (%s) failed to open.", zipFilePath);
		throw FILE_EXCEPTION("Failed to open zip file");
	}

	return std::shared_ptr<ZipFile>(new ZipFile(zipFilePath, unzFile));
}

void ZipFile::ExtractFile(const std::string& path, const std::string& destination) const
{
	if (unzLocateFile(m_unzFile, path.c_str(), 0) != UNZ_OK)
	{
		LOG_INFO_F("Path (%s) in zip file (%s) was not found.", path, m_zipFilePath);
		throw FILE_EXCEPTION("File not found");
	}

	int openResult = unzOpenCurrentFile(m_unzFile);
	if (openResult != UNZ_OK)
	{
		LOG_INFO_F("Failed to open path (%s) in zip file (%s).", path, m_zipFilePath);
		throw FILE_EXCEPTION("Failed to open file");
	}

	std::ofstream destinationFile(destination, std::ios::out | std::ios::binary | std::ios::ate);
	if (!destinationFile.is_open())
	{
		LOG_WARNING_F("Failed to write to destination (%s).", destination);
		throw FILE_EXCEPTION("Failed to write to destination");
	}

	unsigned char buffer[256];
	int readSize;
	while ((readSize = unzReadCurrentFile(m_unzFile, &buffer, 256)) > 0)
	{
		destinationFile.write((const char*)&buffer[0], readSize);
	}

	destinationFile.close();

	unzCloseCurrentFile(m_unzFile);
}

std::vector<std::string> ZipFile::ListFiles() const
{
	std::vector<std::string> files;
	if (unzGoToFirstFile(m_unzFile) == UNZ_OK)
	{
		char buffer[256];
		do
		{
			unz_file_info fileInfo;
			unzGetCurrentFileInfo(m_unzFile, &fileInfo, buffer, 256, NULL, 0, NULL, 0);
			files.push_back(buffer);
		} while (unzGoToNextFile(m_unzFile) == UNZ_OK);
	}

	return files;
}