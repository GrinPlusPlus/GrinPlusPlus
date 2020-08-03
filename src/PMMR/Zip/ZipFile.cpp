#include "ZipFile.h"

#include <Core/Exceptions/FileException.h>
#include <Common/Logger.h>
#include <fstream>

ZipFile::ZipFile(const fs::path& zipFilePath, const unzFile& file)
	: m_zipFilePath(zipFilePath), m_unzFile(file)
{

}

ZipFile::~ZipFile()
{
	unzClose(m_unzFile);
}

std::shared_ptr<ZipFile> ZipFile::Load(const fs::path& zipFilePath)
{
	unzFile unzFile = unzOpen(zipFilePath.u8string().c_str());
	if (unzFile == NULL)
	{
		LOG_ERROR_F("Zip file ({}) failed to open.", zipFilePath);
		throw FILE_EXCEPTION("Failed to open zip file");
	}

	return std::shared_ptr<ZipFile>(new ZipFile(zipFilePath, unzFile));
}

void ZipFile::ExtractFile(const std::string& path, const fs::path& destination) const
{
	if (unzLocateFile(m_unzFile, path.c_str(), 0) != UNZ_OK)
	{
		LOG_INFO_F("Path ({}) in zip file ({}) was not found.", path, m_zipFilePath);
		throw FILE_EXCEPTION("File not found");
	}

	int openResult = unzOpenCurrentFile(m_unzFile);
	if (openResult != UNZ_OK)
	{
		LOG_INFO_F("Failed to open path ({}) in zip file ({}).", path, m_zipFilePath);
		throw FILE_EXCEPTION("Failed to open file");
	}

	std::ofstream destinationFile(destination, std::ios::out | std::ios::binary | std::ios::ate);
	if (!destinationFile.is_open())
	{
		LOG_WARNING_F("Failed to write to destination ({}).", destination);
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