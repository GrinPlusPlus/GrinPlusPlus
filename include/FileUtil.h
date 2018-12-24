#pragma once

#include <string>
#include <fstream>
#include <vector>
#include <filesystem>

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
			return std::filesystem::remove(filePath, ec);
		}

		return false;
	}
};