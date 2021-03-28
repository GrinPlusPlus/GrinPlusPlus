#pragma once

#include <Core/File/File.h>
#include <Common/Util/FileUtil.h>
#include <filesystem.h>
#include <uuid.h>

class TemporaryFile
{
public:
    using Ptr = std::shared_ptr<TemporaryFile>;

    TemporaryFile(const fs::path& path) : m_path(path) { }
    ~TemporaryFile() { FileUtil::RemoveFile(m_path); }

    const fs::path& GetPath() const noexcept { return m_path; }

private:
    fs::path m_path;
};

class TestFileUtil
{
public:
    static TemporaryFile::Ptr CreateTempFile()
    {
        return std::make_shared<TemporaryFile>(fs::temp_directory_path() / uuids::to_string(uuids::uuid_system_generator()()));
    }
};