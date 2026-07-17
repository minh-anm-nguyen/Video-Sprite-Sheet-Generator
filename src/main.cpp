#include <cstdio>
#include <optional>

#include "cli/CliParser.h"
#include "core/AppConfig.h"
#include "core/AppError.h"

int main(int argc, char** argv) {
    try {
        std::optional<vss::AppConfig> config = vss::CliParser::parse(argc, argv);
        if (!config.has_value()) {
            return static_cast<int>(vss::ExitCode::Success);
        }
        std::printf("input:    %s\n", config->inputPath.string().c_str());
        std::printf("grid:     %dx%d, cell width %d px\n", config->cols, config->rows,
                    config->cellWidth);
        if (config->intervalSec.has_value()) {
            std::printf("interval: %g s\n", *config->intervalSec);
        } else {
            std::printf("interval: auto\n");
        }
        std::printf("image:    %s\n", config->imagePath.string().c_str());
        std::printf("vtt:      %s\n", config->vttPath.string().c_str());
        std::printf("quality:  %d\n", config->jpegQuality);
        return static_cast<int>(vss::ExitCode::Success);
    } catch (const vss::AppError& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return static_cast<int>(e.exitCode());
    }
}
