#pragma once

#include "core/AppConfig.h"
#include "core/AppError.h"
#include "core/CancellationToken.h"
#include "interfaces/IFileWriter.h"
#include "interfaces/IVideoSource.h"

namespace vss {

class Application {
public:
    Application(AppConfig config, IVideoSource& videoSource, IFileWriter& fileWriter,
                const CancellationToken* cancellation = nullptr);

    ExitCode run();
    static void reportError(const AppError& error);

private:
    AppConfig config_;
    IVideoSource& videoSource_;
    IFileWriter& fileWriter_;
    const CancellationToken* cancellation_;
};

}
