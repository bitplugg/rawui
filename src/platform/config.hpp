// ============================================================================
//  RAW UI - persisted settings.
//
//  A tiny key=value file.  No JSON library is pulled in because the settings
//  fit in a dozen lines and the format stays human-editable over FTP, which is
//  how most people will poke at it.
// ============================================================================
#pragma once

#include <cstdint>
#include <string>

namespace rawui {

struct AppConfig {
    // Theme
    uint32_t seedRgb = 0x6750A4;
    bool dark = true;
    bool vibrant = false;
    float contrast = 0.0f;
    float typeScale = 1.0f;
    float motionScale = 1.0f;

    // File manager
    bool showHidden = false;
    int sortMode = 0;  // 0 name, 1 size, 2 date

    // Last visited screen index (so the app reopens where you left it).
    int lastScreen = 0;

    std::string path;
};

// Loads settings, creating defaults when nothing is on disk yet.
AppConfig configLoad();

// Writes settings back.  Returns false when the location is not writable.
bool configSave(const AppConfig& config);

// Directory the settings (and future caches) live in.
std::string configDir();

}  // namespace rawui
