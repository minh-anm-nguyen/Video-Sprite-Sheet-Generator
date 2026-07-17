#include "media/VideoFile.h"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>
#include <utility>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/display.h>
}

#include "core/AppError.h"

namespace vss {

namespace {

[[noreturn]] void failInput(const std::string& message) {
    throw AppError(ErrorCategory::Input, message);
}

std::string utf8Path(const std::filesystem::path& path) {
    std::u8string u8 = path.u8string();
    return std::string(u8.begin(), u8.end());
}

int normalizedRotation(double theta) {
    int deg = static_cast<int>(std::lround(theta / 90.0)) * 90 % 360;
    if (deg < 0) {
        deg += 360;
    }
    return deg;
}

int readRotation(const AVStream* stream) {
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(60, 29, 100)
    const AVPacketSideData* sideData =
        av_packet_side_data_get(stream->codecpar->coded_side_data,
                                stream->codecpar->nb_coded_side_data,
                                AV_PKT_DATA_DISPLAYMATRIX);
    const std::uint8_t* matrix = sideData != nullptr ? sideData->data : nullptr;
#else
    const std::uint8_t* matrix =
        av_stream_get_side_data(stream, AV_PKT_DATA_DISPLAYMATRIX, nullptr);
#endif
    if (matrix == nullptr) {
        return 0;
    }
    double theta = av_display_rotation_get(reinterpret_cast<const std::int32_t*>(matrix));
    if (std::isnan(theta)) {
        return 0;
    }
    return normalizedRotation(-theta);
}

struct PacketDeleter {
    void operator()(AVPacket* packet) const {
        av_packet_free(&packet);
    }
};

struct FrameDeleter {
    void operator()(AVFrame* frame) const {
        av_frame_free(&frame);
    }
};

}

VideoFile::~VideoFile() {
    close();
}

VideoFile::VideoFile(VideoFile&& other) noexcept
    : formatCtx_(std::exchange(other.formatCtx_, nullptr)),
      codecCtx_(std::exchange(other.codecCtx_, nullptr)),
      streamIndex_(std::exchange(other.streamIndex_, -1)),
      timeBaseNum_(std::exchange(other.timeBaseNum_, 0)),
      timeBaseDen_(std::exchange(other.timeBaseDen_, 1)) {}

VideoFile& VideoFile::operator=(VideoFile&& other) noexcept {
    if (this != &other) {
        close();
        formatCtx_ = std::exchange(other.formatCtx_, nullptr);
        codecCtx_ = std::exchange(other.codecCtx_, nullptr);
        streamIndex_ = std::exchange(other.streamIndex_, -1);
        timeBaseNum_ = std::exchange(other.timeBaseNum_, 0);
        timeBaseDen_ = std::exchange(other.timeBaseDen_, 1);
    }
    return *this;
}

void VideoFile::close() noexcept {
    if (codecCtx_ != nullptr) {
        avcodec_free_context(&codecCtx_);
    }
    if (formatCtx_ != nullptr) {
        avformat_close_input(&formatCtx_);
    }
    streamIndex_ = -1;
    timeBaseNum_ = 0;
    timeBaseDen_ = 1;
}

double VideoFile::toSeconds(long long timestamp) const noexcept {
    return static_cast<double>(timestamp) * timeBaseNum_ / timeBaseDen_;
}

VideoInfo VideoFile::open(const std::filesystem::path& path) {
    close();

    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        failInput("input file does not exist: " + path.string());
    }
    {
        std::ifstream probe(path, std::ios::binary);
        if (!probe) {
            failInput("cannot read input file: " + path.string());
        }
    }

    if (avformat_open_input(&formatCtx_, utf8Path(path).c_str(), nullptr, nullptr) < 0) {
        failInput("unrecognized or unsupported media format: " + path.string());
    }
    if (avformat_find_stream_info(formatCtx_, nullptr) < 0) {
        close();
        failInput("cannot read stream information: " + path.string());
    }

    const AVCodec* decoder = nullptr;
    int streamIndex =
        av_find_best_stream(formatCtx_, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (streamIndex == AVERROR_STREAM_NOT_FOUND) {
        close();
        failInput("input file has no video stream: " + path.string());
    }
    if (streamIndex < 0 || decoder == nullptr) {
        close();
        failInput("unsupported video codec: " + path.string());
    }
    streamIndex_ = streamIndex;

    AVStream* stream = formatCtx_->streams[streamIndex_];
    codecCtx_ = avcodec_alloc_context3(decoder);
    if (codecCtx_ == nullptr ||
        avcodec_parameters_to_context(codecCtx_, stream->codecpar) < 0 ||
        avcodec_open2(codecCtx_, decoder, nullptr) < 0) {
        close();
        failInput("cannot open video decoder: " + path.string());
    }

    timeBaseNum_ = stream->time_base.num;
    timeBaseDen_ = stream->time_base.den;

    double duration = 0.0;
    if (formatCtx_->duration != AV_NOPTS_VALUE) {
        duration = static_cast<double>(formatCtx_->duration) / AV_TIME_BASE;
    } else if (stream->duration != AV_NOPTS_VALUE) {
        duration = toSeconds(stream->duration);
    } else {
        close();
        failInput("cannot determine video duration: " + path.string());
    }

    int width = stream->codecpar->width;
    int height = stream->codecpar->height;
    int rotation = readRotation(stream);

    AVRational sar = av_guess_sample_aspect_ratio(formatCtx_, stream, nullptr);
    double aspect = static_cast<double>(width) / height;
    if (sar.num > 0 && sar.den > 0) {
        aspect = static_cast<double>(width) * sar.num / (static_cast<double>(height) * sar.den);
    }
    if (rotation == 90 || rotation == 270) {
        aspect = 1.0 / aspect;
    }

    return VideoInfo{duration, width, height, aspect, rotation};
}

bool VideoFile::seekTo(double t) {
    if (formatCtx_ == nullptr || codecCtx_ == nullptr || timeBaseNum_ <= 0) {
        return false;
    }
    long long target =
        std::llround(t * timeBaseDen_ / static_cast<double>(timeBaseNum_));
    if (av_seek_frame(formatCtx_, streamIndex_, target, AVSEEK_FLAG_BACKWARD) < 0) {
        return false;
    }
    avcodec_flush_buffers(codecCtx_);
    return true;
}

std::optional<Frame> VideoFile::decodeFrameAtOrAfter(double t) {
    if (formatCtx_ == nullptr || codecCtx_ == nullptr) {
        return std::nullopt;
    }

    std::unique_ptr<AVPacket, PacketDeleter> packet(av_packet_alloc());
    std::unique_ptr<AVFrame, FrameDeleter> frame(av_frame_alloc());
    if (packet == nullptr || frame == nullptr) {
        return std::nullopt;
    }

    while (true) {
        int recvRet = avcodec_receive_frame(codecCtx_, frame.get());
        if (recvRet == 0) {
            long long best = frame->best_effort_timestamp;
            if (best == AV_NOPTS_VALUE) {
                av_frame_unref(frame.get());
                continue;
            }
            double seconds = toSeconds(best);
            if (seconds < t) {
                av_frame_unref(frame.get());
                continue;
            }
            return Frame(frame.release(), seconds);
        }
        if (recvRet == AVERROR(EAGAIN)) {
            while (true) {
                int readRet = av_read_frame(formatCtx_, packet.get());
                if (readRet < 0) {
                    avcodec_send_packet(codecCtx_, nullptr);
                    break;
                }
                if (packet->stream_index != streamIndex_) {
                    av_packet_unref(packet.get());
                    continue;
                }
                int sendRet = avcodec_send_packet(codecCtx_, packet.get());
                av_packet_unref(packet.get());
                if (sendRet == 0 || sendRet == AVERROR(EAGAIN)) {
                    break;
                }
            }
            continue;
        }
        return std::nullopt;
    }
}

}
