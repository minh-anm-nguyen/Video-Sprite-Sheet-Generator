#include "app/Application.h"

#include <algorithm>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cli/ProgressReporter.h"
#include "core/GridLayout.h"
#include "core/Rect.h"
#include "core/TimestampPlanner.h"
#include "core/VideoInfo.h"
#include "media/Frame.h"
#include "media/FrameScaler.h"
#include "media/ImageEncoder.h"
#include "media/ImageFormat.h"
#include "media/SpriteSheet.h"
#include "output/VttCueBuilder.h"
#include "output/WebVttWriter.h"

namespace vss {

namespace {

struct CancelledError {};

}

Application::Application(AppConfig config, IVideoSource& videoSource,
                         IFileWriter& fileWriter, const CancellationToken* cancellation)
    : config_(std::move(config)),
      videoSource_(videoSource),
      fileWriter_(fileWriter),
      cancellation_(cancellation) {}

ExitCode Application::run() {
    try {
        VideoInfo info = videoSource_.open(config_.inputPath);

        double interval = config_.effectiveInterval(info.duration);
        std::vector<double> timestamps = TimestampPlanner::compute(interval, info.duration);
        int n = static_cast<int>(timestamps.size());

        GridLayout layout(config_.cols, config_.rows, config_.cellWidth, info.aspectRatio);
        int capacity = layout.capacity();
        int sheetCount = (n + capacity - 1) / capacity;

        std::vector<std::filesystem::path> outputs;
        outputs.reserve(static_cast<std::size_t>(sheetCount) + 1);
        for (int s = 0; s < sheetCount; ++s) {
            outputs.push_back(config_.sheetPath(s, sheetCount));
        }
        outputs.push_back(config_.vttPath);
        fileWriter_.precheck(outputs);

        std::optional<ImageFormat> format = formatFromExtension(config_.imagePath);
        if (!format.has_value()) {
            throw AppError(ErrorCategory::Argument,
                           "unsupported image extension: " + config_.imagePath.string());
        }
        FrameScaler scaler(layout.cellWidth(), layout.cellHeight());
        ImageEncoder encoder(*format, config_.jpegQuality);
        ProgressReporter progress(n, config_.quiet);
        int decodedCount = 0;
        int processed = 0;

        for (int s = 0; s < sheetCount; ++s) {
            int begin = s * capacity;
            int count = std::min(capacity, n - begin);
            GridLayout sheetLayout = layout.fitToContent(count);
            SpriteSheet sheet(sheetLayout, s);
            for (int i = 0; i < count; ++i) {
                if (cancellation_ != nullptr && cancellation_->isCancelled()) {
                    throw CancelledError{};
                }
                double t = timestamps[begin + i];
                Rect cell = sheetLayout.cellRect(i);
                if (videoSource_.seekTo(t)) {
                    std::optional<Frame> frame = videoSource_.decodeFrameAtOrAfter(t);
                    if (frame.has_value()) {
                        sheet.place(scaler.normalize(*frame, info.rotationDegrees), cell);
                        ++decodedCount;
                    } else {
                        sheet.fillBlack(cell);
                    }
                } else {
                    sheet.fillBlack(cell);
                }
                progress.update(++processed);
            }
            fileWriter_.write(config_.sheetPath(s, sheetCount), encoder.encode(sheet));
        }
        progress.finish();

        if (decodedCount == 0) {
            fileWriter_.removeAllCreated();
            throw AppError(ErrorCategory::Input,
                           "no frame could be decoded from " + config_.inputPath.string());
        }

        VttCueBuilder builder(config_, layout, sheetCount);
        WebVttWriter(fileWriter_).write(builder.buildCues(n, interval, info.duration),
                                        config_.vttPath);
        return ExitCode::Success;
    } catch (const CancelledError&) {
        fileWriter_.removeAllCreated();
        std::fprintf(stderr, "error: interrupted, all output files were removed\n");
        return ExitCode::Interrupted;
    } catch (const AppError& e) {
        reportError(e);
        return e.exitCode();
    } catch (const std::exception& e) {
        AppError wrapped(ErrorCategory::Processing, e.what());
        reportError(wrapped);
        return wrapped.exitCode();
    }
}

void Application::reportError(const AppError& error) {
    std::fprintf(stderr, "error: %s\n", error.what());
}

}
