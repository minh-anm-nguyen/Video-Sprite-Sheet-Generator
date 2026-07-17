#pragma once

#include <filesystem>
#include <optional>

#include "core/VideoInfo.h"
#include "media/Frame.h"

namespace vss {

class IVideoSource {
public:
    virtual ~IVideoSource() = default;

    virtual VideoInfo open(const std::filesystem::path& path) = 0;
    virtual bool seekTo(double t) = 0;
    virtual std::optional<Frame> decodeFrameAtOrAfter(double t) = 0;
};

}
