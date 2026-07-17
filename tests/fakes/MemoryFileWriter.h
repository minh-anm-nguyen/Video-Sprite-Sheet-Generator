#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "core/AppError.h"
#include "interfaces/IFileWriter.h"

namespace vss {

class MemoryFileWriter : public IFileWriter {
public:
    explicit MemoryFileWriter(bool force = false) : force_(force) {}

    void precheck(const std::vector<std::filesystem::path>& paths) override {
        for (const std::filesystem::path& path : paths) {
            if (files_.contains(path) && !force_) {
                throw AppError(ErrorCategory::Input,
                               "output file already exists: " + path.string() +
                                   ", pass --force to overwrite");
            }
        }
    }

    void write(const std::filesystem::path& path,
               const std::vector<std::uint8_t>& bytes) override {
        files_[path] = bytes;
        created_.push_back(path);
    }

    void removeAllCreated() override {
        for (const std::filesystem::path& path : created_) {
            files_.erase(path);
        }
        created_.clear();
    }

    const std::map<std::filesystem::path, std::vector<std::uint8_t>>& files() const {
        return files_;
    }

    const std::vector<std::filesystem::path>& created() const {
        return created_;
    }

    std::string textOf(const std::filesystem::path& path) const {
        const std::vector<std::uint8_t>& bytes = files_.at(path);
        return std::string(bytes.begin(), bytes.end());
    }

    void preload(const std::filesystem::path& path) {
        files_[path] = {};
    }

private:
    bool force_;
    std::map<std::filesystem::path, std::vector<std::uint8_t>> files_;
    std::vector<std::filesystem::path> created_;
};

}
