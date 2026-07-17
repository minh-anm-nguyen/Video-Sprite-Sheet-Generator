#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include "interfaces/IFileWriter.h"

namespace vss {

class AtomicFileWriter : public IFileWriter {
public:
    explicit AtomicFileWriter(bool force);

    void precheck(const std::vector<std::filesystem::path>& paths) override;
    void write(const std::filesystem::path& path,
               const std::vector<std::uint8_t>& bytes) override;
    void removeAllCreated() override;

    const std::vector<std::filesystem::path>& created() const noexcept;

private:
    static void ensureParentDirs(const std::filesystem::path& path);
    static std::filesystem::path makeTempPath(const std::filesystem::path& target);

    bool force_;
    std::vector<std::filesystem::path> created_;
};

}
