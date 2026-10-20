// ============================================================================
//  RAW UI - theme files on external storage.
//
//  A theme is the same key=value format as config.ini, so the generator on the
//  project site can export a single file that works both as a standalone
//  theme and as a full settings backup.  No JSON parser is pulled in for this
//  on purpose: the format has to stay hand-editable over FTP, and unknown keys
//  are ignored rather than rejected, so files from an older build keep working.
// ============================================================================

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "platform/config.hpp"

namespace rawui {

struct ThemeFile {
    std::string path;
    std::string name;  // file name without the extension, used as the label
    int64_t modified = 0;
};

// Where themes are written by default (first writable USB volume).
std::string defaultThemeDir();

// Every *.ini that carries a theme below the known locations: the USB roots
// plus their RAWUI/, RAWUI/themes/ and themes/ subdirectories, and /data/RAWUI.
// Results are newest first.
std::vector<ThemeFile> scanThemeFiles();

// Applies the theme keys of one file onto `config`.  Files without a seed key
// are rejected - they are not themes, and importing them would silently leave
// the palette unchanged while claiming success.
bool importThemeFile(const std::string& path, AppConfig& config);

// Writes the active theme out.  `outPath` receives the written location when
// the call succeeds.
bool exportTheme(const AppConfig& config, std::string* outPath);

}  // namespace rawui
