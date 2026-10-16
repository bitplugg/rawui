#include "core/log.hpp"

#include <cstdarg>
#include <cstdio>

#include <orbis/libkernel.h>

namespace rawui {

void logPrint(const char* fmt, ...) {
    char buffer[1024];

    va_list args;
    va_start(args, fmt);
    const int n = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (n < 0) return;

    std::fputs(buffer, stdout);
    std::fputc('\n', stdout);
    std::fflush(stdout);

    // The kernel debug channel is the one users watch over the network, so the
    // text has to be passed as data rather than as a format string.
    sceKernelDebugOutText(0, "%s\n", buffer);
}

}  // namespace rawui
