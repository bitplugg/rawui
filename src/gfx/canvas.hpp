// ============================================================================
//  RAW UI - software canvas.
//
//  Everything is rasterised by hand into a 32-bit ARGB frame buffer: rounded
//  rectangles, squircles, circles, rings and soft shadows all get analytic
//  anti-aliasing via 4x vertical supersampling of exact scanline spans.
//
//  The frame buffer is always fully opaque (the background pass covers every
//  pixel), which lets the blend stay a cheap integer lerp.
// ============================================================================
#pragma once

#include <cstdint>
#include <vector>

#include "core/math.hpp"

namespace rawui {

// Source-over blend for an opaque destination.  `a256` is coverage*256 so that
// 256 means "fully opaque" and the maths stays exact.
inline uint32_t blendARGB(uint32_t dst, uint32_t src, uint32_t a256) {
    const uint32_t ia = 256u - a256;
    const uint32_t rb =
        (((src & 0x00FF00FFu) * a256) + ((dst & 0x00FF00FFu) * ia)) >> 8;
    const uint32_t g =
        (((src & 0x0000FF00u) * a256) + ((dst & 0x0000FF00u) * ia)) >> 8;
    return 0xFF000000u | (rb & 0x00FF00FFu) | (g & 0x0000FF00u);
}

inline uint32_t coverageToA256(float coverage, uint8_t colorAlpha) {
    const float a = clampf(coverage, 0.0f, 1.0f) * (static_cast<float>(colorAlpha) / 255.0f);
    return static_cast<uint32_t>(a * 256.0f + 0.5f);
}

inline void blendPixel(uint32_t* p, uint32_t src, uint32_t a256) {
    if (a256 == 0) return;
    if (a256 >= 256) {
        *p = 0xFF000000u | (src & 0x00FFFFFFu);
    } else {
        *p = blendARGB(*p, src, a256);
    }
}

class Canvas {
public:
    Canvas() = default;

    void reset(uint32_t* pixels, int width, int height, int stridePixels);

    int width() const { return width_; }
    int height() const { return height_; }
    int stride() const { return stride_; }
    uint32_t* pixels() const { return pixels_; }

    // ---------------------------------------------------------------- setup --
    void clear(const Color& c);
    void fillRect(const Rect& r, const Color& c);
    void fillGradientV(const Rect& r, const Color& top, const Color& bottom);

    // --------------------------------------------------------------- shapes --
    void fillRoundRect(const Rect& r, float radius, const Color& c);
    void fillRoundRectGradientV(const Rect& r, float radius, const Color& top,
                                const Color& bottom);
    void strokeRoundRect(const Rect& r, float radius, float thickness, const Color& c);
    void fillCircle(float cx, float cy, float radius, const Color& c);
    void fillSquircle(const Rect& r, float n, const Color& c);

    // A capsule from (x0,y0) to (x1,y1).
    void drawLine(float x0, float y0, float x1, float y1, float thickness, const Color& c);
    // Ring segment: angles in degrees, 0 = east, growing clockwise.
    void strokeArc(float cx, float cy, float radius, float thickness, float startDeg,
                   float sweepDeg, const Color& c);

    // Soft drop shadow.  The falloff mask is cached by geometry, so moving a
    // shadow around only costs a blend.
    void shadowRoundRect(const Rect& r, float radius, float blur, const Color& c,
                         float dx = 0.0f, float dy = 0.0f);
    void shadowSquircle(const Rect& r, float n, float blur, const Color& c,
                        float dx = 0.0f, float dy = 0.0f);

    // -------------------------------------------------------------- bitmaps --
    // 8-bit coverage mask (glyph atlas, baked shadows).
    void blendMask(const uint8_t* mask, int maskStride, int mw, int mh, float dstX,
                   float dstY, const Color& color);

    // ----------------------------------------------------------------- clip --
    void pushClip(const Rect& r);
    void popClip();
    Rect clip() const { return clipStack_[clipDepth_ - 1]; }

private:
    // Shared scanline driver.  `extent(dy, halfWidth, halfHeight)` returns the
    // half-width fraction (0..1) of the shape at that vertical offset, which is
    // enough to describe rounded rectangles and superellipses alike.  When
    // `innerInset` is positive the inset shape is subtracted, giving a ring.
    template <typename ExtentFn>
    void fillShapeAA(const Rect& r, float innerInset, const Color& top,
                     const Color& bottom, bool gradient, ExtentFn extent);

    uint32_t* pixels_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    int stride_ = 0;

    std::vector<float> rowCov_;

    static constexpr int kMaxClipDepth = 8;
    Rect clipStack_[kMaxClipDepth];
    int clipDepth_ = 1;

    // --------------------------------------------------------- shadow cache --
    struct ShadowMask {
        uint64_t key = 0;
        int w = 0;
        int h = 0;
        int pad = 0;
        std::vector<uint8_t> alpha;
    };
    std::vector<ShadowMask> shadowCache_;

    const ShadowMask* shadowMask(float w, float h, float radius, float n, float blur,
                                 bool squircle);
    void drawShadow(const ShadowMask* mask, float x, float y, const Color& c);
};

}  // namespace rawui
