#include <cstdio>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

namespace {

void printVersion(const char* name, unsigned version) {
    std::printf("%-12s %u.%u.%u\n", name,
                AV_VERSION_MAJOR(version),
                AV_VERSION_MINOR(version),
                AV_VERSION_MICRO(version));
}

}

int main() {
    std::printf("video-sprite-sheet\n");
    printVersion("libavformat", avformat_version());
    printVersion("libavcodec", avcodec_version());
    printVersion("libavutil", avutil_version());
    printVersion("libswscale", swscale_version());
    return 0;
}
