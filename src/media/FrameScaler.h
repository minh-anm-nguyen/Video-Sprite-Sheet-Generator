#pragma once

#include "media/Frame.h"

struct SwsContext;

namespace vss {

class FrameScaler {
public:
    FrameScaler(int cellWidth, int cellHeight);
    ~FrameScaler();
    FrameScaler(const FrameScaler&) = delete;
    FrameScaler& operator=(const FrameScaler&) = delete;

    Frame normalize(const Frame& frame, int rotationDegrees);

private:
    int cellWidth_;
    int cellHeight_;
    SwsContext* swsCtx_ = nullptr;
};

}
