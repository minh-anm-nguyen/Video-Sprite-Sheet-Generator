#pragma once

struct AVFrame;

namespace vss {

class Frame {
public:
    Frame() noexcept = default;
    Frame(AVFrame* frame, double timestampSec) noexcept;
    Frame(Frame&& other) noexcept;
    Frame& operator=(Frame&& other) noexcept;
    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;
    ~Frame();

    AVFrame* handle() const noexcept;
    double timestampSec() const noexcept;
    int width() const noexcept;
    int height() const noexcept;

private:
    AVFrame* frame_ = nullptr;
    double timestampSec_ = 0.0;
};

}
