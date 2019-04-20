#pragma once

#include <string>
#include <fstream>
#include <vector>
#include <filesystem>
#include <stdlib.h>

#if defined(WIN32)
#include <Windows.h>
#endif

class FileUtil
{
public:
	static bool ReadFile(const std::string& filePath, std::vector<unsigned char>& data)
	{
		if (!std::filesystem::exists(std::filesystem::path(filePath)))
		{
			return false;
		}

		std::ifstream file(filePath, std::ios::in | std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			return false;
		}

		auto size = file.tellg();
		data.resize((size_t)size);

		file.seekg(0, std::ios::beg);
		file.read((char*)&data[0], size);
		file.close();

		return true;
	}

	static bool RenameFile(const std::string& source, const std::string& destination)
	{
		const std::filesystem::path destinationPath(destination);
		if (std::filesystem::exists(destinationPath))
		{
			std::filesystem::remove(destinationPath);
		}

		std::error_code error;
		const std::filesystem::path sourcePath(source);
		std::filesystem::rename(sourcePath, destinationPath, error);

		return !error;
	}

	static bool SafeWriteToFile(const std::string& filePath, const std::vector<unsigned char>& data)
	{
		const std::string tmpFilePath = filePath + ".tmp";
		std::ofstream file(tmpFilePath, std::ios::out | std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			return false;
		}

		file.write((const char*)&data[0], data.size());
		file.close();

		return RenameFile(tmpFilePath, filePath);
	}

	static bool RemoveFile(const std::string& filePath)
	{
		std::error_code ec;
		if (std::filesystem::exists(filePath, ec))
		{
			const uintmax_t removed = std::filesystem::remove_all(filePath, ec);

			return removed > 0 && ec.value() == 0;
		}

		return false;
	}

	static bool CopyDirectory(const std::string& sourceDir, const std::string& destDir)
	{
		std::error_code ec;
		std::filesystem::create_directories(destDir, ec);

		if (!std::filesystem::exists(sourceDir, ec) || !std::filesystem::exists(destDir, ec))
		{
			return false;
		}

		std::filesystem::copy(sourceDir, destDir, std::filesystem::copy_options::recursive, ec);

		return ec.value() == 0;
	}

	static std::string GetHomeDirectory()
	{
		#ifdef _WIN32
		char homeDriveBuf[MAX_PATH_LEN];
		size_t homeDriveLen = MAX_PATH_LEN;
		getenv_s(&homeDriveLen, homeDriveBuf, sizeof(homeDriveBuf) - 1, "HOMEDRIVE");

		char homePathBuf[MAX_PATH_LEN];
		size_t homePathLen = MAX_PATH_LEN;
		getenv_s(&homePathLen, homePathBuf, sizeof(homePathBuf) - 1, "HOMEPATH");

		return std::string(&homeDriveBuf[0]) + std::string(&homePathBuf[0]);
		#else
		char* pHomePath = getenv("HOME");

		return std::string(pHomePath);
		#endif
	}

	static bool TruncateFile(const std::string& filePath, const uint64_t size)
	{
#if defined(WIN32)
		HANDLE hFile = CreateFile(filePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		LARGE_INTEGER li;
		li.QuadPart = size;
		const bool success = SetFilePointerEx(hFile, li, NULL, FILE_BEGIN) && SetEndOfFile(hFile);

		CloseHandle(hFile);

		return success;
#else
		// TODO: Implement
		return true;
#endif
	}

private:
	static const size_t MAX_PATH_LEN = 260;
};
