#pragma once

#include <string>
#include <vector>

#include "core/AppConfig.h"
#include "core/GridLayout.h"
#include "core/VttCue.h"

namespace vss {

class VttCueBuilder {
public:
    VttCueBuilder(const AppConfig& config, const GridLayout& layout, int sheetCount);

    std::vector<VttCue> buildCues(int n, double intervalSec, double durationSec) const;
    std::string makeImageRef(int sheetIndex) const;

private:
    const AppConfig& config_;
    GridLayout layout_;
    int sheetCount_;
};

}
