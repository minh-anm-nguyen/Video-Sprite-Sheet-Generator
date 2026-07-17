#include "output/VttCueBuilder.h"

#include <cassert>
#include <cstddef>

namespace vss {

VttCueBuilder::VttCueBuilder(const AppConfig& config, const GridLayout& layout, int sheetCount)
    : config_(config), layout_(layout), sheetCount_(sheetCount) {
    assert(sheetCount >= 1);
}

std::vector<VttCue> VttCueBuilder::buildCues(int n, double intervalSec,
                                             double durationSec) const {
    assert(n >= 1);
    std::vector<VttCue> cues;
    cues.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        double start = i * intervalSec;
        double end = start + intervalSec;
        if (end > durationSec) {
            end = durationSec;
        }
        int sheetIndex = i / layout_.capacity();
        int localIndex = i % layout_.capacity();
        cues.push_back(
            VttCue{start, end, makeImageRef(sheetIndex), layout_.cellRect(localIndex)});
    }
    return cues;
}

std::string VttCueBuilder::makeImageRef(int sheetIndex) const {
    std::string filename = config_.sheetPath(sheetIndex, sheetCount_).filename().string();
    if (config_.baseUrl.empty()) {
        return filename;
    }
    if (config_.baseUrl.back() == '/') {
        return config_.baseUrl + filename;
    }
    return config_.baseUrl + "/" + filename;
}

}
