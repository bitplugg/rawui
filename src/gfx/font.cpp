#include "gfx/font.hpp"

#include <cstring>

#include "gfx/ft_compat.hpp"

#include <orbis/Sysmodule.h>

#include "core/log.hpp"

namespace rawui {
namespace {

FT_Library g_lib = nullptr;

constexpr int kAtlasWidth = 1024;
constexpr int kAtlasHeight = 512;

}  // namespace

// --------------------------------------------------------------------------
bool fontInit() {
    if (g_lib) return true;

    const int32_t rc = sceSysmoduleLoadModule(ORBIS_SYSMODULE_FREETYPE_OL);
    if (rc != 0) {
        RU_LOG("[font] sceSysmoduleLoadModule(FREETYPE_OL) -> 0x%08X", rc);
        // Keep going: on some setups the module is already resident.
    }

    if (FT_Init_FreeType(&g_lib) != 0 || !g_lib) {
        RU_LOG("[font] FT_Init_FreeType failed");
        g_lib = nullptr;
        return false;
    }

    RU_LOG("[font] FreeType ready");
    return true;
}

void fontShutdown() {
    if (g_lib) {
        FT_Done_FreeType(g_lib);
        g_lib = nullptr;
    }
}

// --------------------------------------------------------------------------
std::vector<uint32_t> utf8Decode(const std::string& text) {
    std::vector<uint32_t> out;
    out.reserve(text.size());

    size_t i = 0;
    const size_t n = text.size();
    while (i < n) {
        const uint8_t c = static_cast<uint8_t>(text[i]);
        uint32_t cp = 0;
        int extra = 0;

        if (c < 0x80) {
            cp = c;
        } else if ((c & 0xE0) == 0xC0) {
            cp = c & 0x1F;
            extra = 1;
        } else if ((c & 0xF0) == 0xE0) {
            cp = c & 0x0F;
            extra = 2;
        } else if ((c & 0xF8) == 0xF0) {
            cp = c & 0x07;
            extra = 3;
        } else {
            ++i;  // invalid lead byte, skip
            continue;
        }

        if (i + extra >= n) break;
        for (int k = 1; k <= extra; ++k) {
            const uint8_t cc = static_cast<uint8_t>(text[i + k]);
            if ((cc & 0xC0) != 0x80) {
                cp = 0;
                break;
            }
            cp = (cp << 6) | (cc & 0x3F);
        }
        i += extra + 1;

        if (cp != 0) out.push_back(cp);
    }

    return out;
}

// --------------------------------------------------------------------------
Font::Font() = default;

Font::~Font() {
    if (face_) FT_Done_Face(face_);
}

bool Font::load(const uint8_t* data, size_t size, float pixelSize) {
    if (!g_lib || !data || size == 0) return false;

    if (FT_New_Memory_Face(g_lib, data, static_cast<FT_Long>(size), 0, &face_) != 0) {
        face_ = nullptr;
        return false;
    }

    if (FT_Select_Charmap(face_, FT_ENCODING_UNICODE) != 0) {
        // Not fatal: many fonts only expose the symbol charmap.
    }

    if (FT_Set_Pixel_Sizes(face_, 0, static_cast<FT_UInt>(pixelSize + 0.5f)) != 0) {
        RU_LOG("[font] FT_Set_Pixel_Sizes(%d) failed", (int)pixelSize);
        return false;
    }

    pixelSize_ = pixelSize;
    ascent_ = static_cast<float>(face_->size->metrics.ascender) / 64.0f;
    descent_ = static_cast<float>(face_->size->metrics.descender) / 64.0f;
    lineHeight_ = static_cast<float>(face_->size->metrics.height) / 64.0f;

    glyphs_.clear();
    atlas_.clear();
    return true;
}

bool Font::setVariation(const char* axisTag, float value) {
    if (!face_ || std::strlen(axisTag) != 4) return false;

    FT_MM_Var* mm = nullptr;
    if (FT_Get_MM_Var(face_, &mm) != 0 || !mm) return false;

    std::vector<FT_Fixed> coords(mm->num_axis);
    for (FT_UInt i = 0; i < mm->num_axis; ++i) coords[i] = mm->axis[i].def;

    const FT_ULong tag = FT_MAKE_TAG(axisTag[0], axisTag[1], axisTag[2], axisTag[3]);
    bool found = false;
    for (FT_UInt i = 0; i < mm->num_axis; ++i) {
        if (mm->axis[i].tag == tag) {
            coords[i] = static_cast<FT_Fixed>(value * 65536.0f);
            found = true;
        }
    }

    bool ok = false;
    if (found) {
        ok = FT_Set_Var_Design_Coordinates(face_, mm->num_axis, coords.data()) == 0;
    }
    // FreeType 2.5 has no FT_Done_MM_Var; the block is small and this runs a
    // handful of times while the UI is starting up.
    (void)mm;

    if (ok) {
        // Outlines changed, so cached bitmaps are stale.
        glyphs_.clear();
        atlas_.clear();
        penX_ = penY_ = rowHeight_ = 0;
        ascent_ = static_cast<float>(face_->size->metrics.ascender) / 64.0f;
        descent_ = static_cast<float>(face_->size->metrics.descender) / 64.0f;
        lineHeight_ = static_cast<float>(face_->size->metrics.height) / 64.0f;
    }
    return ok;
}

// --------------------------------------------------------------------------
void Font::buildAtlas() const {
    if (!atlas_.empty()) return;
    atlasW_ = kAtlasWidth;
    atlasH_ = kAtlasHeight;
    atlas_.assign(static_cast<size_t>(atlasW_) * atlasH_, 0);
    penX_ = 0;
    penY_ = 0;
    rowHeight_ = 0;
}

void Font::rasterise(uint32_t codepoint, GlyphMetrics& out) const {
    out = GlyphMetrics{};
    if (!face_) return;

    FT_Int32 flags = FT_LOAD_TARGET_LIGHT | FT_LOAD_RENDER;
    if (FT_Load_Char(face_, codepoint, flags) != 0) {
        if (FT_Load_Char(face_, codepoint, FT_LOAD_RENDER) != 0) return;
    }

    FT_GlyphSlot slot = face_->glyph;
    const int w = static_cast<int>(slot->bitmap.width);
    const int h = static_cast<int>(slot->bitmap.rows);

    out.advance = static_cast<float>(slot->advance.x) / 64.0f;
    out.bearingX = static_cast<float>(slot->bitmap_left);
    out.bearingY = static_cast<float>(slot->bitmap_top);
    out.width = static_cast<float>(w);
    out.height = static_cast<float>(h);

    if (w <= 0 || h <= 0) {
        out.valid = true;  // e.g. the space glyph: advance only
        return;
    }

    buildAtlas();

    if (penX_ + w > atlasW_) {
        penX_ = 0;
        penY_ += rowHeight_ + 1;
        rowHeight_ = 0;
    }
    if (penY_ + h > atlasH_) {
        out.valid = false;  // atlas full: caller falls back to a direct blit
        return;
    }

    out.ax = penX_;
    out.ay = penY_;
    out.aw = w;
    out.ah = h;

    const int pitch = static_cast<int>(slot->bitmap.pitch);
    for (int y = 0; y < h; ++y) {
        const uint8_t* src = slot->bitmap.buffer + static_cast<size_t>(y) * pitch;
        uint8_t* dst = atlas_.data() + static_cast<size_t>(penY_ + y) * atlasW_ + penX_;
        std::memcpy(dst, src, static_cast<size_t>(w));
    }

    penX_ += w + 1;
    rowHeight_ = std::max(rowHeight_, h);
    out.valid = true;
}

const GlyphMetrics& Font::glyph(uint32_t codepoint) const {
    auto it = glyphs_.find(codepoint);
    if (it != glyphs_.end()) return it->second;

    GlyphMetrics m;
    rasterise(codepoint, m);
    auto res = glyphs_.emplace(codepoint, m);
    return res.first->second;
}

// --------------------------------------------------------------------------
float Font::measure(const std::string& text) const {
    float w = 0.0f;
    for (uint32_t cp : utf8Decode(text)) {
        w += glyph(cp).advance;
    }
    return w;
}

float Font::measureWidth(const std::string& text, float maxWidth, int* lineCount) const {
    const std::vector<uint32_t> cps = utf8Decode(text);
    float lineW = 0.0f;
    float best = 0.0f;
    int lines = 1;
    int wordStart = -1;
    float widthAtWordStart = 0.0f;

    for (size_t i = 0; i < cps.size(); ++i) {
        const uint32_t cp = cps[i];
        if (cp == '\n') {
            best = std::max(best, lineW);
            lineW = 0.0f;
            ++lines;
            wordStart = -1;
            continue;
        }
        if (cp == ' ') wordStart = static_cast<int>(i);

        const float adv = glyph(cp).advance;
        if (maxWidth > 0.0f && lineW + adv > maxWidth && lineW > 0.0f) {
            if (wordStart >= 0) {
                // break at the last space
                ++lines;
                best = std::max(best, widthAtWordStart);
                lineW = lineW - widthAtWordStart + adv;
            } else {
                ++lines;
                best = std::max(best, lineW);
                lineW = adv;
            }
            wordStart = -1;
        } else {
            lineW += adv;
        }
        if (wordStart >= 0) widthAtWordStart = lineW;
    }

    best = std::max(best, lineW);
    if (lineCount) *lineCount = lines;
    return best;
}

void Font::draw(Canvas& canvas, const std::string& text, float x, float y,
                const Color& color, float letterSpacing) const {
    std::vector<uint8_t> temp;

    for (uint32_t cp : utf8Decode(text)) {
        if (cp == '\n') continue;  // single-line draw
        const GlyphMetrics& g = glyph(cp);

        if (g.valid && g.aw > 0 && g.ah > 0) {
            buildAtlas();
            canvas.blendMask(atlas_.data() + static_cast<size_t>(g.ay) * atlasW_ + g.ax,
                             atlasW_, g.aw, g.ah, x + g.bearingX, y - g.bearingY, color);
        } else if (!g.valid && g.width > 0.0f && g.height > 0.0f && face_) {
            // Atlas overflow: rasterise this one glyph into a scratch buffer.
            const int w = static_cast<int>(g.width);
            const int h = static_cast<int>(g.height);
            temp.assign(static_cast<size_t>(w) * h, 0);
            FT_GlyphSlot slot = face_->glyph;
            const int pitch = static_cast<int>(slot->bitmap.pitch);
            for (int row = 0; row < h; ++row) {
                const uint8_t* src = slot->bitmap.buffer + static_cast<size_t>(row) * pitch;
                std::memcpy(temp.data() + static_cast<size_t>(row) * w, src,
                            static_cast<size_t>(w));
            }
            canvas.blendMask(temp.data(), w, w, h, x + g.bearingX, y - g.bearingY, color);
        }

        x += g.advance + letterSpacing;
    }
}

void Font::drawGlyphCentered(Canvas& canvas, uint32_t codepoint, const Rect& box,
                             const Color& color) const {
    const GlyphMetrics& g = glyph(codepoint);
    if (g.width <= 0.0f || g.height <= 0.0f) return;

    // Ink box relative to the pen position: x in [bearingX, bearingX + width],
    // y in [baseline - bearingY, baseline - bearingY + height].
    const float penX = box.cx() - g.bearingX - g.width * 0.5f;
    const float inkTop = box.cy() - g.height * 0.5f;
    const float penY = inkTop + g.bearingY;

    std::vector<uint8_t> temp;

    if (g.valid && g.aw > 0 && g.ah > 0) {
        buildAtlas();
        canvas.blendMask(atlas_.data() + static_cast<size_t>(g.ay) * atlasW_ + g.ax, atlasW_,
                         g.aw, g.ah, penX, inkTop, color);
    } else if (face_) {
        const int w = static_cast<int>(g.width);
        const int h = static_cast<int>(g.height);
        temp.assign(static_cast<size_t>(w) * h, 0);
        FT_GlyphSlot slot = face_->glyph;
        const int pitch = static_cast<int>(slot->bitmap.pitch);
        for (int row = 0; row < h; ++row) {
            const uint8_t* src = slot->bitmap.buffer + static_cast<size_t>(row) * pitch;
            std::memcpy(temp.data() + static_cast<size_t>(row) * w, src, static_cast<size_t>(w));
        }
        canvas.blendMask(temp.data(), w, w, h, penX, inkTop, color);
    }
    (void)penY;
}

int Font::drawWrapped(Canvas& canvas, const std::string& text, const Rect& box,
                      const Color& color, float lineHeight, float letterSpacing) const {
    const std::vector<uint32_t> cps = utf8Decode(text);

    std::string line;
    float lineW = 0.0f;
    float y = box.y + ascent_;
    int lines = 0;

    auto flush = [&]() {
        if (!line.empty()) {
            draw(canvas, line, box.x, y, color, letterSpacing);
        }
        y += lineHeight;
        ++lines;
        line.clear();
        lineW = 0.0f;
    };

    for (size_t i = 0; i < cps.size(); ++i) {
        const uint32_t cp = cps[i];

        if (cp == '\n') {
            flush();
            continue;
        }

        const float adv = glyph(cp).advance + letterSpacing;

        if (cp == ' ' && lineW + adv > box.w && !line.empty()) {
            flush();
            continue;
        }

        if (lineW + adv > box.w && !line.empty()) {
            // Backtrack to the last space so words are not cut in half.
            const size_t space = line.find_last_of(' ');
            if (space != std::string::npos && space > 0) {
                const std::string carry = line.substr(space + 1);
                line.resize(space);
                draw(canvas, line, box.x, y, color, letterSpacing);
                y += lineHeight;
                ++lines;
                line = carry;
                lineW = measure(line);
            } else {
                draw(canvas, line, box.x, y, color, letterSpacing);
                y += lineHeight;
                ++lines;
                line.clear();
                lineW = 0.0f;
            }
        }

        // re-encode the code point
        if (cp < 0x80) {
            line.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            line.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            line.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            line.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            line.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            line.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
        lineW += adv;

        if (y > box.bottom()) break;
    }

    if (!line.empty()) {
        draw(canvas, line, box.x, y, color, letterSpacing);
        ++lines;
    }

    return lines;
}

}  // namespace rawui
