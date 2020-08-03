#include "Zipper.h"

#include <Common/Util/FileUtil.h>
#include <Core/File/File.h>
#include <Core/Exceptions/FileException.h>
#include <Common/Logger.h>
#include <fstream>
#include <filesystem.h>

void Zipper::CreateZipFile(const fs::path& destination, const std::vector<fs::path>& paths)
{
	zipFile zf = zipOpen(destination.u8string().c_str(), APPEND_STATUS_CREATE);
	if (zf == nullptr)
	{
		LOG_ERROR_F("Failed to create zip file at ({})", destination);
		throw FILE_EXCEPTION_F("Failed to create zip file at ({})", destination);
	}

	try
	{
		for (size_t i = 0; i < paths.size(); i++)
		{
			if (fs::is_directory(paths[i]))
			{
				AddDirectory(zf, paths[i], paths[i].filename());
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
		throw;
	}

	if (zipClose(zf, NULL))
	{
		LOG_ERROR_F("Failed to close zip file ({})", destination);
		throw FILE_EXCEPTION_F("Failed to close zip file ({})", destination);
	}
}

void Zipper::AddDirectory(zipFile zf, const fs::path& sourceDir, const fs::path& destDir)
{
	for (const auto& entry : fs::directory_iterator(sourceDir))
	{
		if (fs::is_directory(entry))
		{
			AddDirectory(zf, entry, destDir / entry.path().filename());
		}
		else
		{
			AddFile(zf, entry.path().string(), destDir);
		}
	}
}

void Zipper::AddFile(zipFile zf, const fs::path& sourceFile, const fs::path& destDir)
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
			fs::path fileName = destDir / sourceFile.filename();

			if (ZIP_OK == zipOpenNewFileInZip(zf, fileName.string().c_str(), &zfi, nullptr, 0, nullptr, 0, nullptr, 0, Z_NO_COMPRESSION))
			{
				if (zipWriteInFileInZip(zf, size == 0 ? "" : buffer.data(), (unsigned int)size))
				{
					throw FILE_EXCEPTION_F("Failed to write to file {}", fileName);
				}

				if (zipCloseFileInZip(zf))
				{
					throw FILE_EXCEPTION_F("Failed to close file {}", fileName);
				}

				return;
			}
		}
	}

	throw FILE_EXCEPTION_F("Failed to add file {}", destDir);
}