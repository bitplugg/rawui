// ============================================================================
//  RAW UI - GoldHEN awareness.
//
//  GoldHEN does not publish a userland status API, so the app works the way the
//  scene does: look at the well-known files on disk and knock on the ports the
//  payload's bundled servers listen on.
// ============================================================================
#pragma once

#include <string>
#include <vector>

namespace rawui {

struct HenServer {
    const char* key;
    const char* label;
    const char* description;
    const char* host;
    int port;
    bool listening;
};

struct HenStatus {
    bool payloadDirPresent = false;
    bool running = false;
    int portCount = 0;
    std::string version;
    std::string payloadPath;
    std::vector<HenServer> servers;

    int cheatCount = 0;
    int patchCount = 0;
    std::string cheatDir;
    std::string patchDir;
};

// Refreshes the status.  The port probes have short timeouts, so this takes a
// few tens of milliseconds at most.
HenStatus probeHen();

// Single TCP connect test (used for the port probes, also handy in Settings).
bool tcpProbe(const char* host, int port, int timeoutMs);

// Directory helpers used by the cheats screen.
std::vector<std::string> cheatDirectories();
std::vector<std::string> patchDirectories();

}  // namespace rawui
