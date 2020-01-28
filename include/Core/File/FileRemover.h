#pragma once

#include <Common/Util/FileUtil.h>

// Removes a file when object goes out of scope
class FileRemover
{
public:
	FileRemover(const fs::path& path) : m_path(path) {}
	~FileRemover() { FileUtil::RemoveFile(m_path); }

private:
	fs::path m_path;
};