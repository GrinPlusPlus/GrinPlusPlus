#pragma once

#include <mutex>
#include <memory>
#include <vector>
#include <cstdint>
#include <filesystem.h>

class IMappedFile
{
public:
    using UPtr = std::unique_ptr<IMappedFile>;

    static IMappedFile::UPtr Load(const fs::path& path);
    virtual ~IMappedFile() = default;

    virtual bool Write(const size_t startIndex, const std::vector<uint8_t>& data) = 0;
    virtual void Read(const uint64_t position, const uint64_t numBytes, std::vector<uint8_t>& data) const = 0;
};

