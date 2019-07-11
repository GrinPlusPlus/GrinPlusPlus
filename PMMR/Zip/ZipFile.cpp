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
		LoggerAPI::LogError("ZipFile::Open - Zip file (" + m_zipFilePath + ") failed to open.");
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
		LoggerAPI::LogWarning("ZipFile::ExtractFile - Zip file (" + m_zipFilePath + ") is not open.");
		return EZipFileStatus::NOT_OPEN;
	}

	if (unzLocateFile(m_unzFile, path.c_str(), 0) != UNZ_OK)
	{
		LoggerAPI::LogInfo("ZipFile::ExtractFile - Path (" + path + ") in zip file (" + m_zipFilePath + ") was not found.");
		return EZipFileStatus::NOT_FOUND;
	}

	int openResult = unzOpenCurrentFile(m_unzFile);
	if (openResult != UNZ_OK)
	{
		LoggerAPI::LogInfo("ZipFile::ExtractFile - Failed to open path (" + path + ") in zip file (" + m_zipFilePath + ").");
		return EZipFileStatus::NOT_FOUND;
	}

	std::ofstream destinationFile(destination, std::ios::out | std::ios::binary | std::ios::ate);
	if (!destinationFile.is_open())
	{
		LoggerAPI::LogWarning("ZipFile::ExtractFile - Failed to write to destination (" + destination + ").");
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
		LoggerAPI::LogWarning("ZipFile::ListFiles - Zip file (" + m_zipFilePath + ") is not open.");
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