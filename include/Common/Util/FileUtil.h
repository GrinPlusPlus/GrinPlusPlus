#pragma once

#include <Common/Util/StringUtil.h>
#include <filesystem.h>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <Windows.h>
#define SEPARATOR "\\"
#else
#include <unistd.h>
#define SEPARATOR "/"
#endif

class FileUtil
{
  public:
    static bool ReadFile(const std::string &filePath, std::vector<unsigned char> &data)
    {
        if (!fs::exists(fs::path(filePath)))
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
        file.read((char *)&data[0], size);
        file.close();

        return true;
    }

    static bool RenameFile(const std::string &source, const std::string &destination)
    {
        const fs::path destinationPath(destination);
        if (fs::exists(destinationPath))
        {
            fs::remove(destinationPath);
        }

        std::error_code error;
        const fs::path sourcePath(source);
        fs::rename(sourcePath, destinationPath, error);

        return !error;
    }

    static bool SafeWriteToFile(const std::string &filePath, const std::vector<unsigned char> &data)
    {
        const std::string tmpFilePath = filePath + ".tmp";
        std::ofstream file(tmpFilePath, std::ios::out | std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            return false;
        }

        file.write((const char *)&data[0], data.size());
        file.close();

        return RenameFile(tmpFilePath, filePath);
    }

    static bool RemoveFile(const std::string &filePath)
    {
        std::error_code ec;
        if (fs::exists(filePath, ec))
        {
            const uintmax_t removed = fs::remove_all(filePath, ec);

            return removed > 0 && ec.value() == 0;
        }

        return false;
    }

    static bool CopyDirectory(const std::string &sourceDir, const std::string &destDir)
    {
        std::error_code ec;
        fs::create_directories(destDir, ec);

        if (!fs::exists(sourceDir, ec) || !fs::exists(destDir, ec))
        {
            return false;
        }

        fs::copy(sourceDir, destDir, fs::copy_options::recursive, ec);

        return ec.value() == 0;
    }

    static bool CreateDirectories(const std::string &directory)
    {
        std::error_code ec;
        return fs::create_directories(directory, ec);
    }

    static bool Exists(const std::string &path)
    {
        std::error_code ec;
        if (fs::exists(path, ec))
        {
            return true;
        }

        return false;
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
        char *pHomePath = getenv("HOME");
        if (pHomePath != nullptr)
        {
            return std::string(pHomePath);
        }
        else
        {
            return "~";
        }
#endif
    }

    static bool TruncateFile(const std::string &filePath, const uint64_t size)
    {
#if defined(WIN32)
        HANDLE hFile = CreateFile(filePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL, NULL);

        LARGE_INTEGER li;
        li.QuadPart = size;
        const bool success = SetFilePointerEx(hFile, li, NULL, FILE_BEGIN) && SetEndOfFile(hFile);

        CloseHandle(hFile);

        return success;
#else
        return truncate(filePath.c_str(), size) == 0;
#endif
    }

    static std::vector<std::string> GetSubDirectories(const std::string &filePath, const bool includeHidden)
    {
        std::vector<std::string> listOfFiles;
        try
        {
            // Check if given path exists and points to a directory
            if (fs::exists(filePath) && fs::is_directory(filePath))
            {
                // Create a Recursive Directory Iterator object and points to the starting of directory
                fs::recursive_directory_iterator iter(filePath);

                // Create a Recursive Directory Iterator object pointing to end.
                fs::recursive_directory_iterator end;

                // Iterate till end
                while (iter != end)
                {
                    // Check if current entry is a directory and if exists in skip list
                    if (fs::is_directory(iter->path()))
                    {
                        // Skip the iteration of current directory pointed by iterator
                        // c++17 fstem API to skip current directory iteration
                        iter.disable_recursion_pending();
                        std::string filename = iter->path().filename().string();
                        if (includeHidden || !StringUtil::StartsWith(filename, "."))
                        {
                            listOfFiles.push_back(filename);
                        }
                    }

                    std::error_code ec;
                    // Increment the iterator to point to next entry in recursive iteration
                    iter.increment(ec);
                    if (ec)
                    {
                        // std::cerr << "Error While Accessing : " << iter->path().string() << " :: " << ec.message() <<
                        // '\n';
                    }
                }
            }
        }
        catch (std::system_error &)
        {
            // std::cerr << "Exception :: " << e.what();
        }

        return listOfFiles;
    }

  private:
    static const size_t MAX_PATH_LEN = 260;
};
