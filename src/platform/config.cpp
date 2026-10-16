#include "platform/config.hpp"

#include <cstdlib>
#include <cstring>
#include <unordered_map>

#include "core/log.hpp"
#include "core/math.hpp"
#include "platform/fs.hpp"

namespace rawui {
namespace {

const char* kPrimaryDir = "/data/RAWUI";
const char* kFallbackDir = "/user/data/RAWUI";

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

std::string readWholeFile(const std::string& path) {
    std::FILE* fh = std::fopen(path.c_str(), "rb");
    if (!fh) return {};

    std::string out;
    char buf[1024];
    size_t n = 0;
    while ((n = std::fread(buf, 1, sizeof(buf), fh)) > 0) out.append(buf, n);
    std::fclose(fh);
    return out;
}

}  // namespace

std::string configDir() {
    if (fs::isDir(kPrimaryDir)) return kPrimaryDir;
    if (fs::exists("/data") && fs::makeDir(kPrimaryDir)) return kPrimaryDir;
    if (fs::exists("/user/data") && fs::makeDir(kFallbackDir)) return kFallbackDir;
    return kPrimaryDir;
}

AppConfig configLoad() {
    AppConfig cfg;
    cfg.path = joinPath(configDir(), "config.ini");

    const std::string text = readWholeFile(cfg.path);
    if (text.empty()) {
        RU_LOG("[config] no config at %s, using defaults", cfg.path.c_str());
        return cfg;
    }

    const auto kv = parseIni(text);

    const auto get = [&kv](const char* key) -> std::string {
        auto it = kv.find(key);
        return it == kv.end() ? std::string() : it->second;
    };

    cfg.seedRgb = static_cast<uint32_t>(std::strtoul(get("seed").c_str(), nullptr, 16));
    if (cfg.seedRgb == 0) cfg.seedRgb = 0x6750A4;

    cfg.dark = parseBool(get("dark"), cfg.dark);
    cfg.vibrant = parseBool(get("vibrant"), cfg.vibrant);
    cfg.contrast = parseFloat(get("contrast"), cfg.contrast);
    cfg.typeScale = parseFloat(get("type_scale"), cfg.typeScale);
    cfg.motionScale = parseFloat(get("motion_scale"), cfg.motionScale);
    cfg.showHidden = parseBool(get("show_hidden"), cfg.showHidden);
    cfg.sortMode = parseInt(get("sort_mode"), cfg.sortMode);
    cfg.lastScreen = parseInt(get("last_screen"), cfg.lastScreen);

    cfg.typeScale = clampf(cfg.typeScale, 0.8f, 1.4f);
    cfg.motionScale = clampf(cfg.motionScale, 0.0f, 1.0f);
    cfg.contrast = clampf(cfg.contrast, -1.0f, 1.0f);

    RU_LOG("[config] loaded from %s", cfg.path.c_str());
    return cfg;
}

bool configSave(const AppConfig& config) {
    const std::string dir = dirName(config.path);
    if (!dir.empty()) fs::makeDir(dir);

    std::FILE* fh = std::fopen(config.path.c_str(), "wb");
    if (!fh) {
        RU_LOG("[config] cannot write %s", config.path.c_str());
        return false;
    }

    std::fprintf(fh, "# RAW UI settings\n");
    std::fprintf(fh, "seed=%06X\n", config.seedRgb & 0xFFFFFF);
    std::fprintf(fh, "dark=%d\n", config.dark ? 1 : 0);
    std::fprintf(fh, "vibrant=%d\n", config.vibrant ? 1 : 0);
    std::fprintf(fh, "contrast=%.2f\n", config.contrast);
    std::fprintf(fh, "type_scale=%.2f\n", config.typeScale);
    std::fprintf(fh, "motion_scale=%.2f\n", config.motionScale);
    std::fprintf(fh, "show_hidden=%d\n", config.showHidden ? 1 : 0);
    std::fprintf(fh, "sort_mode=%d\n", config.sortMode);
    std::fprintf(fh, "last_screen=%d\n", config.lastScreen);

    std::fclose(fh);
    return true;
}

}  // namespace rawui
