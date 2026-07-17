#include "output/AtomicFileWriter.h"

#include <cstdio>
#include <fstream>
#include <random>
#include <string>
#include <system_error>

#include "core/AppError.h"

namespace vss {

namespace {

std::string randomSuffix() {
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<std::uint64_t> dist;
    char buffer[17];
    std::snprintf(buffer, sizeof(buffer), "%016llx",
                  static_cast<unsigned long long>(dist(rng)));
    return std::string(buffer, 16);
}

}

AtomicFileWriter::AtomicFileWriter(bool force) : force_(force) {}

void AtomicFileWriter::precheck(const std::vector<std::filesystem::path>& paths) {
    if (force_) {
        return;
    }
    for (const std::filesystem::path& path : paths) {
        std::error_code ec;
        if (std::filesystem::exists(path, ec)) {
            throw AppError(ErrorCategory::Input,
                           "output file already exists: " + path.string() +
                               ", pass --force to overwrite");
        }
    }
}

void AtomicFileWriter::ensureParentDirs(const std::filesystem::path& path) {
    std::filesystem::path parent = path.parent_path();
    if (parent.empty()) {
        return;
    }
    std::error_code ec;
    std::filesystem::create_directories(parent, ec);
    if (ec) {
        throw AppError(ErrorCategory::Processing,
                       "cannot create directory " + parent.string() + ": " + ec.message());
    }
}

std::filesystem::path AtomicFileWriter::makeTempPath(const std::filesystem::path& target) {
    std::filesystem::path temp = target;
    temp += ".tmp" + randomSuffix();
    return temp;
}

void AtomicFileWriter::write(const std::filesystem::path& path,
                             const std::vector<std::uint8_t>& bytes) {
    ensureParentDirs(path);
    std::filesystem::path temp = makeTempPath(path);
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) {
            throw AppError(ErrorCategory::Processing,
                           "cannot create file " + temp.string());
        }
        if (!bytes.empty()) {
            out.write(reinterpret_cast<const char*>(bytes.data()),
                      static_cast<std::streamsize>(bytes.size()));
        }
        out.flush();
        if (!out) {
            out.close();
            std::error_code ec;
            std::filesystem::remove(temp, ec);
            throw AppError(ErrorCategory::Processing,
                           "cannot write file " + temp.string());
        }
    }
    std::error_code ec;
    std::filesystem::rename(temp, path, ec);
    if (ec) {
        std::error_code removeEc;
        std::filesystem::remove(temp, removeEc);
        throw AppError(ErrorCategory::Processing,
                       "cannot rename " + temp.string() + " to " + path.string() + ": " +
                           ec.message());
    }
    created_.push_back(path);
}

void AtomicFileWriter::removeAllCreated() {
    for (const std::filesystem::path& path : created_) {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
    created_.clear();
}

const std::vector<std::filesystem::path>& AtomicFileWriter::created() const noexcept {
    return created_;
}

}
