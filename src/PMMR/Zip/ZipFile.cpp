#include "ZipFile.h"

#include <Infrastructure/Logger.h>
#include <fstream>

ZipFile::ZipFile(const std::string& zipFilePath)
	: m_zipFilePath(zipFilePath)
{
}

EZipFileStatus ZipFile::Open()
{
	m_unzFile = unzOpen(m_zipFilePath.c_str());
	if (m_unzFile == NULL)
	{
		LOG_ERROR_F("Zip file (%s) failed to open.", m_zipFilePath);
		return EZipFileStatus::NOT_FOUND;
	}

	return EZipFileStatus::SUCCESS;
}

void ZipFile::Close()
{
	unzClose(m_unzFile);
	m_unzFile = NULL;
}

EZipFileStatus ZipFile::ExtractFile(const std::string& path, const std::string& destination) const
{   
	if (m_unzFile == NULL)
	{
		LOG_WARNING_F("Zip file (%s) is not open.", m_zipFilePath);
		return EZipFileStatus::NOT_OPEN;
	}

	if (unzLocateFile(m_unzFile, path.c_str(), 0) != UNZ_OK)
	{
		LOG_INFO_F("Path (%s) in zip file (%s) was not found.", path, m_zipFilePath);
		return EZipFileStatus::NOT_FOUND;
	}

	int openResult = unzOpenCurrentFile(m_unzFile);
	if (openResult != UNZ_OK)
	{
		LOG_INFO_F("Failed to open path (%s) in zip file (%s).", path, m_zipFilePath);
		return EZipFileStatus::NOT_FOUND;
	}

	std::ofstream destinationFile(destination, std::ios::out | std::ios::binary | std::ios::ate);
	if (!destinationFile.is_open())
	{
		LOG_WARNING_F("Failed to write to destination (%s).", destination);
		return EZipFileStatus::WRITE_FAILED;
	}

	unsigned char buffer[256];
	int readSize;
	while ((readSize = unzReadCurrentFile(m_unzFile, &buffer, 256)) > 0)
	{
		destinationFile.write((const char*)&buffer[0], readSize);
	}

	destinationFile.close();

	unzCloseCurrentFile(m_unzFile);

	return EZipFileStatus::SUCCESS;
}

EZipFileStatus ZipFile::ListFiles(std::vector<std::string>& files) const
{
    if (m_unzFile == NULL)
	{
		LOG_WARNING_F("Zip file (%s) is not open.", m_zipFilePath);
		return EZipFileStatus::NOT_OPEN;
    }

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

	return EZipFileStatus::SUCCESS;
}