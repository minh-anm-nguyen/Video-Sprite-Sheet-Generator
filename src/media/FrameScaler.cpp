#include "media/FrameScaler.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

#include "core/AppError.h"

namespace vss {

namespace {

[[noreturn]] void failProcessing(const std::string& message) {
    throw AppError(ErrorCategory::Processing, message);
}

struct FrameDeleter {
    void operator()(AVFrame* frame) const {
        av_frame_free(&frame);
    }
};

using FramePtr = std::unique_ptr<AVFrame, FrameDeleter>;

FramePtr allocRgbFrame(int width, int height) {
    FramePtr frame(av_frame_alloc());
    if (frame == nullptr) {
        failProcessing("cannot allocate frame");
    }
    frame->format = AV_PIX_FMT_RGB24;
    frame->width = width;
    frame->height = height;
    if (av_frame_get_buffer(frame.get(), 0) < 0) {
        failProcessing("cannot allocate frame buffer");
    }
    return frame;
}

FramePtr rotateRgb(const AVFrame* src, int degrees) {
    int w = src->width;
    int h = src->height;
    int dw = degrees == 180 ? w : h;
    int dh = degrees == 180 ? h : w;
    FramePtr dst = allocRgbFrame(dw, dh);
    for (int y = 0; y < dh; ++y) {
        std::uint8_t* dstRow = dst->data[0] + y * dst->linesize[0];
        for (int x = 0; x < dw; ++x) {
            int sx = 0;
            int sy = 0;
            if (degrees == 90) {
                sx = y;
                sy = h - 1 - x;
            } else if (degrees == 180) {
                sx = w - 1 - x;
                sy = h - 1 - y;
            } else {
                sx = w - 1 - y;
                sy = x;
            }
            const std::uint8_t* srcPixel = src->data[0] + sy * src->linesize[0] + sx * 3;
            std::memcpy(dstRow + x * 3, srcPixel, 3);
        }
    }
    return dst;
}

}

FrameScaler::FrameScaler(int cellWidth, int cellHeight)
    : cellWidth_(cellWidth), cellHeight_(cellHeight) {}

FrameScaler::~FrameScaler() {
    sws_freeContext(swsCtx_);
}

Frame FrameScaler::normalize(const Frame& frame, int rotationDegrees) {
    const AVFrame* src = frame.handle();
    if (src == nullptr || src->width <= 0 || src->height <= 0) {
        failProcessing("cannot scale an empty frame");
    }

    bool swapped = rotationDegrees == 90 || rotationDegrees == 270;
    int rotatedW = swapped ? src->height : src->width;
    int rotatedH = swapped ? src->width : src->height;
    double factor = std::min(static_cast<double>(cellWidth_) / rotatedW,
                             static_cast<double>(cellHeight_) / rotatedH);
    int targetW = std::max(1, static_cast<int>(std::lround(rotatedW * factor)));
    int targetH = std::max(1, static_cast<int>(std::lround(rotatedH * factor)));
    int preW = swapped ? targetH : targetW;
    int preH = swapped ? targetW : targetH;

    FramePtr scaled = allocRgbFrame(preW, preH);
    swsCtx_ = sws_getCachedContext(swsCtx_, src->width, src->height,
                                   static_cast<AVPixelFormat>(src->format), preW, preH,
                                   AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr,
                                   nullptr);
    if (swsCtx_ == nullptr) {
        failProcessing("cannot create scaler context");
    }
    if (sws_scale(swsCtx_, src->data, src->linesize, 0, src->height, scaled->data,
                  scaled->linesize) != preH) {
        failProcessing("cannot scale frame");
    }

    FramePtr oriented =
        rotationDegrees != 0 ? rotateRgb(scaled.get(), rotationDegrees) : std::move(scaled);

    if (targetW == cellWidth_ && targetH == cellHeight_) {
        return Frame(oriented.release(), frame.timestampSec());
    }

    FramePtr cell = allocRgbFrame(cellWidth_, cellHeight_);
    std::memset(cell->data[0], 0,
                static_cast<std::size_t>(cell->linesize[0]) * cellHeight_);
    int offsetX = (cellWidth_ - targetW) / 2;
    int offsetY = (cellHeight_ - targetH) / 2;
    for (int y = 0; y < targetH; ++y) {
        std::memcpy(cell->data[0] + (offsetY + y) * cell->linesize[0] + offsetX * 3,
                    oriented->data[0] + y * oriented->linesize[0],
                    static_cast<std::size_t>(targetW) * 3);
    }
    return Frame(cell.release(), frame.timestampSec());
}

}
