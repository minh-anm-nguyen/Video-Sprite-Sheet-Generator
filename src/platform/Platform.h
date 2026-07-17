#pragma once

#include "core/CancellationToken.h"

namespace vss::platform {

bool stdoutIsTerminal();
void installCancellationHandler(CancellationToken& token);

}
