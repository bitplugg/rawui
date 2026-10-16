// ============================================================================
//  RAW UI - text rendering.
//
//  FreeType comes from the console itself (libSceFreeType), so no font engine
//  is bundled.  Each Font owns one FT_Face plus a lazily filled 8-bit glyph
//  atlas; variable-font weights are selected through the "wght" design axis and
//  Material Symbols' filled style through "FILL".
// ============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/math.hpp"
#include "gfx/canvas.hpp"

// FreeType handle, forward declared so the header stays dependency free.  The
// real declarations come from gfx/ft_compat.hpp in the implementation.
struct FT_FaceRec_;
struct FT_LibraryRec_;

namespace rawui {

// Shared FreeType library handle (loads the module once for the whole app).
bool fontInit();
void fontShutdown();

struct GlyphMetrics {
    float advance = 0.0f;
    float bearingX = 0.0f;
    float bearingY = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    // Atlas placement (only valid when `valid`).
    int ax = 0, ay = 0;
    int aw = 0, ah = 0;
    bool valid = false;
};

class Font {
public:
    Font();
    ~Font();

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    // `data` must stay alive for the lifetime of the Font.
    bool load(const uint8_t* data, size_t size, float pixelSize);
    bool setVariation(const char* axisTag, float value);

    float pixelSize() const { return pixelSize_; }
    float ascent() const { return ascent_; }
    float descent() const { return descent_; }
    float lineHeight() const { return lineHeight_; }

    // Text measurement / drawing.  Both understand UTF-8, which is how icon
    // code points reach the rasteriser.
    float measure(const std::string& text) const;
    float measureWidth(const std::string& text, float maxWidth, int* lineCount) const;

    void draw(Canvas& canvas, const std::string& text, float x, float y,
              const Color& color, float letterSpacing = 0.0f) const;
    // Draws a single glyph centred inside `box` (used for the icon font, whose
    // glyphs are square but not flush with the em box).
    void drawGlyphCentered(Canvas& canvas, uint32_t codepoint, const Rect& box,
                           const Color& color) const;
    // Draws wrapped text and returns the number of lines emitted.
    int drawWrapped(Canvas& canvas, const std::string& text, const Rect& box,
                    const Color& color, float lineHeight, float letterSpacing = 0.0f) const;

private:
    const GlyphMetrics& glyph(uint32_t codepoint) const;
    void buildAtlas() const;
    void rasterise(uint32_t codepoint, GlyphMetrics& out) const;

    FT_FaceRec_* face_ = nullptr;
    float pixelSize_ = 16.0f;
    float ascent_ = 0.0f;
    float descent_ = 0.0f;
    float lineHeight_ = 0.0f;

    mutable std::unordered_map<uint32_t, GlyphMetrics> glyphs_;
    mutable std::vector<uint8_t> atlas_;
    mutable int atlasW_ = 0;
    mutable int atlasH_ = 0;
    mutable int penX_ = 0;
    mutable int penY_ = 0;
    mutable int rowHeight_ = 0;
};

// UTF-8 helpers.
std::vector<uint32_t> utf8Decode(const std::string& text);

}  // namespace rawui
