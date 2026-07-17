#include <optional>

extern "C" {
#include <libavutil/log.h>
}

#include "app/Application.h"
#include "cli/CliParser.h"
#include "core/AppConfig.h"
#include "core/AppError.h"
#include "media/VideoFile.h"
#include "output/AtomicFileWriter.h"

int main(int argc, char** argv) {
    av_log_set_level(AV_LOG_ERROR);
    std::optional<vss::AppConfig> config;
    try {
        config = vss::CliParser::parse(argc, argv);
    } catch (const vss::AppError& e) {
        vss::Application::reportError(e);
        return static_cast<int>(e.exitCode());
    }
    if (!config.has_value()) {
        return static_cast<int>(vss::ExitCode::Success);
    }

    vss::VideoFile videoSource;
    vss::AtomicFileWriter fileWriter(config->force);
    vss::Application app(*config, videoSource, fileWriter);
    return static_cast<int>(app.run());
}
