#include "platform/config.hpp"

#include <cstdlib>
#include <ctime>
#include <unordered_map>

#include "core/log.hpp"
#include "core/math.hpp"
#include "platform/fs.hpp"

namespace rawui {
namespace {

const char* kPrimaryDir = "/data/RAWUI";
const char* kFallbackDir = "/user/data/RAWUI";

// Where the USB round-trip lives.  Keeping it under its own directory means a
// drive that also holds PKGs and saves is not polluted with a stray file.
const char* kBackupDirName = "RAWUI";
const char* kBackupFileName = "config.ini";

constexpr int kMaxMinute = 23 * 60 + 59;

std::unordered_map<std::string, std::string> parseIni(const std::string& text) {
    std::unordered_map<std::string, std::string> out;

    size_t pos = 0;
    while (pos < text.size()) {
        size_t eol = text.find('\n', pos);
        if (eol == std::string::npos) eol = text.size();

        std::string line = text.substr(pos, eol - pos);
        pos = eol + 1;

        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();

        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);

        // Trim.
        while (!key.empty() && key.back() == ' ') key.pop_back();
        while (!value.empty() && value.front() == ' ') value.erase(0, 1);

        if (!key.empty()) out[key] = value;
    }

    return out;
}

bool parseBool(const std::string& v, bool fallback) {
    if (v.empty()) return fallback;
    return v == "1" || v == "true" || v == "yes" || v == "on";
}

float parseFloat(const std::string& v, float fallback) {
    if (v.empty()) return fallback;
    return static_cast<float>(std::atof(v.c_str()));
}

int parseInt(const std::string& v, int fallback) {
    if (v.empty()) return fallback;
    return std::atoi(v.c_str());
}

}  // namespace

std::string configDir() {
    if (fs::isDir(kPrimaryDir)) return kPrimaryDir;
    if (fs::exists("/data") && fs::makeDir(kPrimaryDir)) return kPrimaryDir;
    if (fs::exists("/user/data") && fs::makeDir(kFallbackDir)) return kFallbackDir;
    return kPrimaryDir;
}

// ---------------------------------------------------------------------------
void configApplyIni(AppConfig& config, const std::string& text) {
    const auto kv = parseIni(text);

    const auto get = [&kv](const char* key) -> std::string {
        auto it = kv.find(key);
        return it == kv.end() ? std::string() : it->second;
    };

    // Absent keys leave the current value alone.  That is what makes this the
    // shared path for "load config.ini" and "import a theme file": a theme
    // file only carries the theme keys and must not wipe the rest.
    const std::string seed = get("seed");
    if (!seed.empty()) {
        const uint32_t parsed =
            static_cast<uint32_t>(std::strtoul(seed.c_str(), nullptr, 16));
        // An all-zero seed would collapse the palette onto greys, so treat it
        // as a typo rather than honouring it.
        if ((parsed & 0xFFFFFF) != 0) config.seedRgb = parsed & 0xFFFFFF;
    }

    config.dark = parseBool(get("dark"), config.dark);
    config.vibrant = parseBool(get("vibrant"), config.vibrant);
    config.contrast = parseFloat(get("contrast"), config.contrast);
    config.typeScale = parseFloat(get("type_scale"), config.typeScale);
    config.motionScale = parseFloat(get("motion_scale"), config.motionScale);
    config.showHidden = parseBool(get("show_hidden"), config.showHidden);
    config.sortMode = parseInt(get("sort_mode"), config.sortMode);
    config.lastScreen = parseInt(get("last_screen"), config.lastScreen);

    config.lang = langFromName(get("lang").c_str());

    config.nightMode = parseBool(get("night_mode"), config.nightMode);
    config.nightStart = clampi(parseInt(get("night_start"), config.nightStart), 0, kMaxMinute);
    config.nightEnd = clampi(parseInt(get("night_end"), config.nightEnd), 0, kMaxMinute);

    config.typeScale = clampf(config.typeScale, 0.8f, 1.4f);
    config.motionScale = clampf(config.motionScale, 0.0f, 1.0f);
    config.contrast = clampf(config.contrast, -1.0f, 1.0f);
}

std::string configSerialize(const AppConfig& config) {
    char buf[512];
    const int written = std::snprintf(
        buf, sizeof(buf),
        "# RAW UI settings\n"
        "seed=%06X\n"
        "dark=%d\n"
        "vibrant=%d\n"
        "contrast=%.2f\n"
        "type_scale=%.2f\n"
        "motion_scale=%.2f\n"
        "lang=%s\n"
        "night_mode=%d\n"
        "night_start=%d\n"
        "night_end=%d\n"
        "show_hidden=%d\n"
        "sort_mode=%d\n"
        "last_screen=%d\n",
        config.seedRgb & 0xFFFFFF, config.dark ? 1 : 0, config.vibrant ? 1 : 0,
        static_cast<double>(config.contrast), static_cast<double>(config.typeScale),
        static_cast<double>(config.motionScale), langTag(config.lang),
        config.nightMode ? 1 : 0, clampi(config.nightStart, 0, kMaxMinute),
        clampi(config.nightEnd, 0, kMaxMinute), config.showHidden ? 1 : 0, config.sortMode,
        config.lastScreen);

    if (written <= 0) return {};
    const size_t len = static_cast<size_t>(written);
    return std::string(buf, len < sizeof(buf) ? len : sizeof(buf) - 1);
}

AppConfig configLoad() {
    AppConfig cfg;
    cfg.path = joinPath(configDir(), "config.ini");

    const std::string text = fs::readFile(cfg.path);
    if (text.empty()) {
        RU_LOG("[config] no config at %s, using defaults", cfg.path.c_str());
        return cfg;
    }

    configApplyIni(cfg, text);
    RU_LOG("[config] loaded from %s", cfg.path.c_str());
    return cfg;
}

bool configSave(const AppConfig& config) {
    // A default-constructed AppConfig (or a copy that lost its path) has no
    // location; falling back to the canonical one means saving still works
    // instead of silently doing nothing.
    const std::string path =
        config.path.empty() ? joinPath(configDir(), "config.ini") : config.path;

    const std::string dir = dirName(path);
    if (!dir.empty()) fs::makeDir(dir);

    if (!fs::writeFile(path, configSerialize(config))) {
        RU_LOG("[config] cannot write %s", path.c_str());
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
bool nightWindowActive(const AppConfig& config, std::time_t now) {
    if (!config.nightMode || now <= 0) return false;

    struct tm parts;
    if (localtime_r(&now, &parts) == nullptr) return false;

    const int minute = clampi(parts.tm_hour * 60 + parts.tm_min, 0, kMaxMinute);
    const int start = clampi(config.nightStart, 0, kMaxMinute);
    const int end = clampi(config.nightEnd, 0, kMaxMinute);

    if (start == end) return false;  // degenerate window, treat as "off"
    if (start < end) return minute >= start && minute < end;

    // Wraps past midnight, which is the common case (22:00 -> 08:00).
    return minute >= start || minute < end;
}

bool configBackupUsb(const AppConfig& config, std::string* outPath) {
    for (const std::string& root : fs::usbRoots()) {
        const std::string dir = joinPath(root, kBackupDirName);
        fs::makeDir(dir);  // may fail if it already exists; the write below is
                           // what actually matters.

        const std::string path = joinPath(dir, kBackupFileName);
        if (fs::writeFile(path, configSerialize(config))) {
            if (outPath) *outPath = path;
            RU_LOG("[config] backup written to %s", path.c_str());
            return true;
        }
        RU_LOG("[config] could not write %s", path.c_str());
    }
    RU_LOG("[config] backup failed: no writable USB volume");
    return false;
}

bool configRestoreUsb(AppConfig& config, std::string* outPath) {
    for (const std::string& root : fs::usbRoots()) {
        // Accept the canonical location plus a file dropped at the volume
        // root, which is what someone copying it by hand is most likely to do.
        const std::string candidates[] = {joinPath(joinPath(root, kBackupDirName), kBackupFileName),
                                          joinPath(root, kBackupFileName)};

        for (const std::string& path : candidates) {
            const std::string text = fs::readFile(path);
            if (text.empty()) continue;

            // Keep the on-disk location of the live config: restoring must
            // not redirect future saves onto the USB stick.
            configApplyIni(config, text);
            if (outPath) *outPath = path;
            RU_LOG("[config] restored from %s", path.c_str());
            return true;
        }
    }
    RU_LOG("[config] restore failed: no settings backup found on USB");
    return false;
}

}  // namespace rawui
