#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace vss {

class IFileWriter {
public:
    virtual ~IFileWriter() = default;

    virtual void precheck(const std::vector<std::filesystem::path>& paths) = 0;
    virtual void write(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes) = 0;
    virtual void removeAllCreated() = 0;
};

}
