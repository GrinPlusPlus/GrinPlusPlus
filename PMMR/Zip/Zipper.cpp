#include "Zipper.h"

#include <fstream>
#include <filesystem.h>

bool Zipper::CreateZipFile(const std::string& destination, const std::vector<std::string>& paths)
{
	zipFile zf = zipOpen(destination.c_str(), APPEND_STATUS_CREATE);
	if (zf == nullptr)
	{
		return false;
	}

	bool _return = true;
	for (size_t i = 0; i < paths.size(); i++)
	{
		if (fs::is_directory(fs::path(paths[i])))
		{
			const std::string destinationPath = paths[i].substr(std::max(paths[i].rfind('\\'), paths[i].rfind('/')) + 1);
			if (!AddDirectory(zf, paths[i], destinationPath))
			{
				_return = false;
				break;
			}
		}
		else if (!AddFile(zf, paths[i], destination))
		{
			_return = false;
			break;
		}
	}

	if (zipClose(zf, NULL))
	{
		return false;
	}

	return _return;
}

bool Zipper::AddDirectory(zipFile zf, const fs::path& sourceDir, const std::string& destDir)
{
	for (const auto& entry : fs::directory_iterator(sourceDir))
	{
		if (fs::is_directory(entry))
		{
			if (!AddDirectory(zf, entry, destDir + "/" + entry.path().filename().string()))
			{
				return false;
			}
		}
		else
		{
			if (!AddFile(zf, entry.path().string(), destDir))
			{
				return false;
			}
		}
	}

	return true;
}

bool Zipper::AddFile(zipFile zf, const std::string& sourceFile, const std::string& destDir)
{
	std::fstream file(sourceFile.c_str(), std::ios::binary | std::ios::in);
	if (file.is_open())
	{
		file.seekg(0, std::ios::end);
		long size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<char> buffer(size);
		if (size == 0 || file.read(&buffer[0], size))
		{
			zip_fileinfo zfi = { 0 };
			std::string fileName = destDir + "/" + sourceFile.substr(std::max(sourceFile.rfind('\\'), sourceFile.rfind('/')) + 1);

			if (ZIP_OK == zipOpenNewFileInZip(zf, fileName.c_str(), &zfi, nullptr, 0, nullptr, 0, nullptr, 0, Z_NO_COMPRESSION))
			{
				if (zipWriteInFileInZip(zf, size == 0 ? "" : &buffer[0], size))
				{
					return false;
				}

				if (zipCloseFileInZip(zf))
				{
					return false;
				}

				file.close();
				return true;
			}
		}
		file.close();
	}

	return false;
}