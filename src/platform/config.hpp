// ============================================================================
//  RAW UI - persisted settings.
//
//  A tiny key=value file.  No JSON library is pulled in because the settings
//  fit in a dozen lines and the format stays human-editable over FTP, which is
//  how most people will poke at it.
// ============================================================================
#pragma once

#include <cstdint>
#include <ctime>
#include <string>

#include "core/i18n.hpp"

namespace rawui {

struct AppConfig {
    // Theme
    uint32_t seedRgb = 0x6750A4;
    bool dark = true;
    bool vibrant = false;
    float contrast = 0.0f;
    float typeScale = 1.0f;
    float motionScale = 1.0f;

    // Interface language.
    Lang lang = Lang::English;

    // Night mode: force the dark theme inside [nightStart, nightEnd) local
    // time.  Bounds are minutes since midnight, so a window may wrap past
    // midnight - which is the normal case (22:00 -> 08:00).
    bool nightMode = false;
    int nightStart = 22 * 60;
    int nightEnd = 8 * 60;

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

// Applies key=value text (the on-disk format) on top of `config`.  Keys that
// are absent keep their current value, so this doubles as the theme-import
// path: a theme file only carries the theme keys and must not wipe the rest.
void configApplyIni(AppConfig& config, const std::string& text);

// The on-disk format as a string - what configSave writes and what the USB
// backup carries, so both sides always agree on the schema.
std::string configSerialize(const AppConfig& config);

// True when night mode should currently be in effect.
bool nightWindowActive(const AppConfig& config, std::time_t now);

// Round-trips config.ini to a mounted USB volume as RAWUI/config.ini, so a
// reinstall or a second console can pick the settings up.  Both return false
// when no volume is mounted, and set *outPath on success.
bool configBackupUsb(const AppConfig& config, std::string* outPath);
bool configRestoreUsb(AppConfig& config, std::string* outPath);

}  // namespace rawui
