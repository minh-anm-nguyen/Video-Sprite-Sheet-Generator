#include "media/ImageEncoder.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

#include "core/AppError.h"

namespace vss {

namespace {

[[noreturn]] void failProcessing(const std::string& message) {
    throw AppError(ErrorCategory::Processing, message);
}

struct CodecCtxDeleter {
    void operator()(AVCodecContext* ctx) const {
        avcodec_free_context(&ctx);
    }
};

struct FrameDeleter {
    void operator()(AVFrame* frame) const {
        av_frame_free(&frame);
    }
};

struct PacketDeleter {
    void operator()(AVPacket* packet) const {
        av_packet_free(&packet);
    }
};

struct SwsDeleter {
    void operator()(SwsContext* ctx) const {
        sws_freeContext(ctx);
    }
};

}

ImageEncoder::ImageEncoder(ImageFormat format, int quality)
    : format_(format), quality_(quality) {}

std::vector<std::uint8_t> ImageEncoder::encode(const SpriteSheet& sheet) const {
    bool jpeg = format_ == ImageFormat::Jpeg;
    const AVCodec* codec =
        avcodec_find_encoder(jpeg ? AV_CODEC_ID_MJPEG : AV_CODEC_ID_PNG);
    if (codec == nullptr) {
        failProcessing("image encoder not available");
    }

    std::unique_ptr<AVCodecContext, CodecCtxDeleter> ctx(avcodec_alloc_context3(codec));
    if (ctx == nullptr) {
        failProcessing("cannot allocate image encoder context");
    }
    ctx->width = sheet.width();
    ctx->height = sheet.height();
    ctx->time_base = AVRational{1, 25};
    if (jpeg) {
        ctx->pix_fmt = AV_PIX_FMT_YUVJ420P;
        ctx->color_range = AVCOL_RANGE_JPEG;
        ctx->flags |= AV_CODEC_FLAG_QSCALE;
        int clamped = std::clamp(quality_, 1, 100);
        ctx->global_quality = FF_QP2LAMBDA * (2 + (100 - clamped) * 29 / 99);
    } else {
        ctx->pix_fmt = AV_PIX_FMT_RGB24;
    }
    if (avcodec_open2(ctx.get(), codec, nullptr) < 0) {
        failProcessing("cannot open image encoder");
    }

    std::unique_ptr<AVFrame, FrameDeleter> frame(av_frame_alloc());
    if (frame == nullptr) {
        failProcessing("cannot allocate image frame");
    }
    frame->format = ctx->pix_fmt;
    frame->width = ctx->width;
    frame->height = ctx->height;
    if (av_frame_get_buffer(frame.get(), 0) < 0) {
        failProcessing("cannot allocate image frame buffer");
    }

    const std::uint8_t* srcData[4] = {sheet.data(), nullptr, nullptr, nullptr};
    int srcLinesize[4] = {sheet.stride(), 0, 0, 0};
    if (jpeg) {
        std::unique_ptr<SwsContext, SwsDeleter> sws(
            sws_getContext(sheet.width(), sheet.height(), AV_PIX_FMT_RGB24, ctx->width,
                           ctx->height, ctx->pix_fmt, SWS_BILINEAR, nullptr, nullptr,
                           nullptr));
        if (sws == nullptr) {
            failProcessing("cannot create canvas converter");
        }
        frame->color_range = AVCOL_RANGE_JPEG;
        if (sws_scale(sws.get(), srcData, srcLinesize, 0, sheet.height(), frame->data,
                      frame->linesize) != sheet.height()) {
            failProcessing("cannot convert canvas for jpeg");
        }
        frame->quality = ctx->global_quality;
    } else {
        for (int y = 0; y < sheet.height(); ++y) {
            std::memcpy(frame->data[0] + static_cast<std::size_t>(y) * frame->linesize[0],
                        sheet.data() + static_cast<std::size_t>(y) * sheet.stride(),
                        static_cast<std::size_t>(sheet.width()) * 3);
        }
    }

    if (avcodec_send_frame(ctx.get(), frame.get()) < 0) {
        failProcessing("cannot encode image");
    }
    avcodec_send_frame(ctx.get(), nullptr);

    std::unique_ptr<AVPacket, PacketDeleter> packet(av_packet_alloc());
    if (packet == nullptr) {
        failProcessing("cannot allocate image packet");
    }
    std::vector<std::uint8_t> bytes;
    while (true) {
        int ret = avcodec_receive_packet(ctx.get(), packet.get());
        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
            break;
        }
        if (ret < 0) {
            failProcessing("cannot read encoded image");
        }
        bytes.insert(bytes.end(), packet->data, packet->data + packet->size);
        av_packet_unref(packet.get());
    }
    if (bytes.empty()) {
        failProcessing("image encoder produced no data");
    }
    return bytes;
}

}
