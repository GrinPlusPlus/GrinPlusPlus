#pragma once

#include <Core/File/File.h>
#include <Common/Util/FileUtil.h>
#include <filesystem.h>

class TemporaryFile
{
public:
    TemporaryFile(const fs::path& path) : m_path(path) { }
    ~TemporaryFile() { FileUtil::RemoveFile(m_path); }


};

class TestFileUtil
{
public:

};