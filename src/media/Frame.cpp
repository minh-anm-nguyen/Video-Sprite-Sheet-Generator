#include "media/Frame.h"

extern "C" {
#include <libavutil/frame.h>
}

namespace vss {

Frame::Frame(AVFrame* frame, double timestampSec) noexcept
    : frame_(frame), timestampSec_(timestampSec) {}

Frame::Frame(Frame&& other) noexcept
    : frame_(other.frame_), timestampSec_(other.timestampSec_) {
    other.frame_ = nullptr;
    other.timestampSec_ = 0.0;
}

Frame& Frame::operator=(Frame&& other) noexcept {
    if (this != &other) {
        av_frame_free(&frame_);
        frame_ = other.frame_;
        timestampSec_ = other.timestampSec_;
        other.frame_ = nullptr;
        other.timestampSec_ = 0.0;
    }
    return *this;
}

Frame::~Frame() {
    av_frame_free(&frame_);
}

AVFrame* Frame::handle() const noexcept {
    return frame_;
}

double Frame::timestampSec() const noexcept {
    return timestampSec_;
}

int Frame::width() const noexcept {
    return frame_ != nullptr ? frame_->width : 0;
}

int Frame::height() const noexcept {
    return frame_ != nullptr ? frame_->height : 0;
}

}
