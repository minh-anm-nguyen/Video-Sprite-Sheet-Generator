#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "core/VttCue.h"
#include "interfaces/IFileWriter.h"

namespace vss {

class WebVttWriter {
public:
    explicit WebVttWriter(IFileWriter& fileWriter);

    static std::string formatTimestamp(double seconds);
    static std::string serialize(const std::vector<VttCue>& cues);
    void write(const std::vector<VttCue>& cues, const std::filesystem::path& path) const;

private:
    IFileWriter& fileWriter_;
};

}
