#include "Zipper.h"

#include <Common/Util/FileUtil.h>
#include <Core/File.h>
#include <Core/Exceptions/FileException.h>
#include <Infrastructure/Logger.h>
#include <fstream>
#include <filesystem.h>

bool Zipper::CreateZipFile(const std::string& destination, const std::vector<std::string>& paths)
{
	zipFile zf = zipOpen(destination.c_str(), APPEND_STATUS_CREATE);
	if (zf == nullptr)
	{
		LOG_ERROR_F("Failed to create zip file at ({})", destination);
		return false;
	}

	try
	{
		for (size_t i = 0; i < paths.size(); i++)
		{
			if (fs::is_directory(fs::path(paths[i])))
			{
				const std::string destinationPath = paths[i].substr(std::max(paths[i].rfind('\\'), paths[i].rfind('/')) + 1);
				AddDirectory(zf, paths[i], destinationPath);
			}
			else
			{
				AddFile(zf, paths[i], destination);
			}
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Failed to create zip file: {}", e.what());
		zipClose(zf, NULL);
		return false;
	}

	if (zipClose(zf, NULL))
	{
		LOG_ERROR_F("Failed to close zip file ({})", destination);
		return false;
	}

	return true;
}

void Zipper::AddDirectory(zipFile zf, const fs::path& sourceDir, const std::string& destDir)
{
	for (const auto& entry : fs::directory_iterator(sourceDir))
	{
		if (fs::is_directory(entry))
		{
			AddDirectory(zf, entry, destDir + "/" + entry.path().filename().string());
		}
		else
		{
			AddFile(zf, entry.path().string(), destDir);
		}
	}
}

void Zipper::AddFile(zipFile zf, const std::string& sourceFile, const std::string& destDir)
{
	File pFile = File::Load(sourceFile, std::ios::binary | std::ios::in);
	if (pFile->is_open())
	{
		size_t size = FileUtil::GetFileSize(sourceFile);
		pFile->seekg(0, std::ios::beg);

		std::vector<char> buffer(size);
		if (size == 0 || pFile->read(&buffer[0], size))
		{
			zip_fileinfo zfi;
			std::string fileName = destDir + "/" + sourceFile.substr((std::max)(sourceFile.rfind('\\'), sourceFile.rfind('/')) + 1);

			if (ZIP_OK == zipOpenNewFileInZip(zf, fileName.c_str(), &zfi, nullptr, 0, nullptr, 0, nullptr, 0, Z_NO_COMPRESSION))
			{
				if (zipWriteInFileInZip(zf, size == 0 ? "" : &buffer[0], (unsigned int)size))
				{
					throw FILE_EXCEPTION(StringUtil::Format("Failed to write to file {}", fileName));
				}

				if (zipCloseFileInZip(zf))
				{
					throw FILE_EXCEPTION(StringUtil::Format("Failed to close file {}", fileName));
				}

				return;
			}
		}
	}

	throw FILE_EXCEPTION(StringUtil::Format("Failed to add file {}", destDir));
}