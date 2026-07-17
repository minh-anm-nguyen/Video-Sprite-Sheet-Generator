#include "platform/Platform.h"

#ifdef _WIN32
#include <cstdio>
#include <io.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <csignal>
#include <cstdio>
#include <unistd.h>
#endif

namespace vss::platform {

namespace {

CancellationToken* gToken = nullptr;

#ifdef _WIN32
BOOL WINAPI consoleHandler(DWORD ctrlType) {
    switch (ctrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        if (gToken != nullptr) {
            gToken->requestCancel();
        }
        return TRUE;
    case CTRL_CLOSE_EVENT:
        if (gToken != nullptr) {
            gToken->requestCancel();
        }
        Sleep(2500);
        return TRUE;
    default:
        return FALSE;
    }
}
#else
void signalHandler(int) {
    if (gToken != nullptr) {
        gToken->requestCancel();
    }
}
#endif

}

bool stdoutIsTerminal() {
#ifdef _WIN32
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(fileno(stdout)) != 0;
#endif
}

void installCancellationHandler(CancellationToken& token) {
    gToken = &token;
#ifdef _WIN32
    SetConsoleCtrlHandler(consoleHandler, TRUE);
#else
    struct sigaction action {};
    action.sa_handler = signalHandler;
    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGTERM, &action, nullptr);
#endif
}

}
