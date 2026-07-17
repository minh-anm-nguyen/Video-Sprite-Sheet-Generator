#include "output/WebVttWriter.h"

#include <cmath>
#include <cstdint>

namespace vss {

namespace {

std::string padded(long long value, int width) {
    std::string text = std::to_string(value);
    while (static_cast<int>(text.size()) < width) {
        text.insert(text.begin(), '0');
    }
    return text;
}

}

WebVttWriter::WebVttWriter(IFileWriter& fileWriter) : fileWriter_(fileWriter) {}

std::string WebVttWriter::formatTimestamp(double seconds) {
    long long totalMs = std::llround(seconds * 1000.0);
    if (totalMs < 0) {
        totalMs = 0;
    }
    long long ms = totalMs % 1000;
    long long totalSec = totalMs / 1000;
    long long sec = totalSec % 60;
    long long min = (totalSec / 60) % 60;
    long long hours = totalSec / 3600;
    return padded(hours, 2) + ":" + padded(min, 2) + ":" + padded(sec, 2) + "." +
           padded(ms, 3);
}

std::string WebVttWriter::serialize(const std::vector<VttCue>& cues) {
    std::string out = "WEBVTT\n\n";
    for (const VttCue& cue : cues) {
        out += formatTimestamp(cue.start) + " --> " + formatTimestamp(cue.end) + "\n";
        out += cue.imageRef + "#xywh=" + std::to_string(cue.region.x) + "," +
               std::to_string(cue.region.y) + "," + std::to_string(cue.region.w) + "," +
               std::to_string(cue.region.h) + "\n\n";
    }
    return out;
}

void WebVttWriter::write(const std::vector<VttCue>& cues,
                         const std::filesystem::path& path) const {
    std::string text = serialize(cues);
    fileWriter_.write(path, std::vector<std::uint8_t>(text.begin(), text.end()));
}

}
