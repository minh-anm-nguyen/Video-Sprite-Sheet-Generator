#pragma once

#include <optional>

#include "core/AppConfig.h"

namespace vss {

class CliParser {
public:
    static std::optional<AppConfig> parse(int argc, const char* const* argv);
    static void printHelp();

private:
    static void resolveOutputPaths(AppConfig& config);
    static void validate(const AppConfig& config);
};

}
