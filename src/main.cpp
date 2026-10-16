// ============================================================================
//  RAW UI - entry point.
// ============================================================================

#include <SDL2/SDL.h>

#include "app/app.hpp"
#include "core/log.hpp"

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    rawui::App app;
    if (!app.init()) {
        RU_LOG("[main] app.init() failed");
        return 1;
    }

    app.run();
    app.shutdown();
    return 0;
}
