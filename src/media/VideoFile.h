#pragma once

#include <filesystem>
#include <optional>

#include "core/VideoInfo.h"
#include "interfaces/IVideoSource.h"
#include "media/Frame.h"

struct AVFormatContext;
struct AVCodecContext;

namespace vss {

class VideoFile : public IVideoSource {
public:
    VideoFile() noexcept = default;
    ~VideoFile() override;
    VideoFile(const VideoFile&) = delete;
    VideoFile& operator=(const VideoFile&) = delete;
    VideoFile(VideoFile&& other) noexcept;
    VideoFile& operator=(VideoFile&& other) noexcept;

    VideoInfo open(const std::filesystem::path& path) override;
    bool seekTo(double t) override;
    std::optional<Frame> decodeFrameAtOrAfter(double t) override;

private:
    void close() noexcept;
    double toSeconds(long long timestamp) const noexcept;

    AVFormatContext* formatCtx_ = nullptr;
    AVCodecContext* codecCtx_ = nullptr;
    int streamIndex_ = -1;
    int timeBaseNum_ = 0;
    int timeBaseDen_ = 1;
};

}
