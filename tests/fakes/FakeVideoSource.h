#pragma once

#include <cstring>
#include <filesystem>
#include <optional>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
}

#include "core/AppError.h"
#include "core/VideoInfo.h"
#include "interfaces/IVideoSource.h"
#include "media/Frame.h"

namespace vss {

class FakeVideoSource : public IVideoSource {
public:
    VideoInfo info{100.0, 640, 360, 16.0 / 9.0, 0};
    double failFrom = 1e18;
    bool failOpen = false;
    bool failSeek = false;
    int openCalls = 0;
    int decodeCalls = 0;

    VideoInfo open(const std::filesystem::path& path) override {
        ++openCalls;
        if (failOpen) {
            throw AppError(ErrorCategory::Input,
                           "input file does not exist: " + path.string());
        }
        return info;
    }

    bool seekTo(double) override {
        return !failSeek;
    }

    std::optional<Frame> decodeFrameAtOrAfter(double t) override {
        ++decodeCalls;
        if (t >= failFrom) {
            return std::nullopt;
        }
        AVFrame* frame = av_frame_alloc();
        if (frame == nullptr) {
            return std::nullopt;
        }
        frame->format = AV_PIX_FMT_RGB24;
        frame->width = info.width;
        frame->height = info.height;
        if (av_frame_get_buffer(frame, 0) < 0) {
            av_frame_free(&frame);
            return std::nullopt;
        }
        for (int y = 0; y < frame->height; ++y) {
            std::memset(frame->data[0] + static_cast<std::size_t>(y) * frame->linesize[0],
                        180, static_cast<std::size_t>(frame->width) * 3);
        }
        return Frame(frame, t);
    }
};

}
