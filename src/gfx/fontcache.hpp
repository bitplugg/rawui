// ============================================================================
//  RAW UI - shared font loading.
//
//  The font files are read once into memory (from /app0/assets/fonts) and every
//  (face, size, weight) combination gets its own Font instance.  Instances are
//  created on demand and then reused, which keeps the number of FreeType faces
//  and glyph atlases small.
// ============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "gfx/font.hpp"

namespace rawui {

class FontCache {
public:
    bool init(const std::string& assetDir);
    void shutdown();

    bool ready() const { return !textData_.empty() && !iconData_.empty(); }

    // Roboto Flex at the requested pixel size and weight (wght axis).
    Font& text(float size, int weight);

    // Material Symbols Rounded; `filled` drives the FILL axis.
    Font& icon(float size, bool filled);

    const std::string& errors() const { return errors_; }

private:
    struct Entry {
        int kind = 0;  // 0 = text, 1 = icon
        float size = 0.0f;
        int weight = 400;
        bool filled = false;
        std::unique_ptr<Font> font;
    };

    static std::vector<uint8_t> readFile(const std::string& path, std::string& errors);

    std::vector<uint8_t> textData_;
    std::vector<uint8_t> iconData_;
    std::vector<Entry> entries_;
    std::string errors_;
};

}  // namespace rawui
