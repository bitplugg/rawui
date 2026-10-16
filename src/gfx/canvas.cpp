#include "gfx/canvas.hpp"

namespace rawui {
namespace {

// Signed distance to a rounded rectangle centred on the origin.
inline float sdfRoundRect(float px, float py, float hw, float hh, float rad) {
    const float qx = std::fabs(px) - (hw - rad);
    const float qy = std::fabs(py) - (hh - rad);
    const float ax = maxf(qx, 0.0f);
    const float ay = maxf(qy, 0.0f);
    return std::sqrt(ax * ax + ay * ay) + minf(maxf(qx, qy), 0.0f) - rad;
}

// Approximate distance to a superellipse (squircle when n == 4).
inline float sdfSquircle(float px, float py, float hw, float hh, float n) {
    const float fx = std::pow(std::fabs(px) / hw, n);
    const float fy = std::pow(std::fabs(py) / hh, n);
    const float f = std::pow(fx + fy, 1.0f / n) - 1.0f;
    return f * minf(hw, hh);
}

inline uint64_t hashShape(float w, float h, float radius, float n, float blur,
                          bool squircle) {
    auto mix = [](uint64_t seed, uint64_t v) {
        seed ^= v + 0x9E3779B97F4A7C15ull + (seed << 6) + (seed >> 2);
        return seed;
    };
    uint64_t k = 0xCBF29CE484222325ull;
    k = mix(k, static_cast<uint64_t>(w * 16.0f + 0.5f));
    k = mix(k, static_cast<uint64_t>(h * 16.0f + 0.5f));
    k = mix(k, static_cast<uint64_t>(radius * 16.0f + 0.5f));
    k = mix(k, static_cast<uint64_t>(n * 16.0f + 0.5f));
    k = mix(k, static_cast<uint64_t>(blur * 16.0f + 0.5f));
    k = mix(k, squircle ? 1u : 0u);
    return k;
}

}  // namespace

// --------------------------------------------------------------------------
void Canvas::reset(uint32_t* pixels, int width, int height, int stridePixels) {
    pixels_ = pixels;
    width_ = width;
    height_ = height;
    stride_ = stridePixels;
    rowCov_.assign(static_cast<size_t>(maxf(1.0f, static_cast<float>(width))), 0.0f);
    clipStack_[0] = Rect(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    clipDepth_ = 1;
}

void Canvas::pushClip(const Rect& r) {
    const Rect next = clip().intersect(r);
    if (clipDepth_ < kMaxClipDepth) {
        clipStack_[clipDepth_] = next;
        ++clipDepth_;
    } else {
        clipStack_[kMaxClipDepth - 1] = next;
    }
}

void Canvas::popClip() {
    if (clipDepth_ > 1) --clipDepth_;
}

// --------------------------------------------------------------------------
void Canvas::clear(const Color& c) {
    const uint32_t v = c.argb();
    const bool opaque = c.a >= 255;
    for (int y = 0; y < height_; ++y) {
        uint32_t* row = pixels_ + static_cast<size_t>(y) * stride_;
        if (opaque) {
            const uint32_t solid = 0xFF000000u | (v & 0x00FFFFFFu);
            for (int x = 0; x < width_; ++x) row[x] = solid;
        } else {
            const uint32_t a256 = coverageToA256(1.0f, c.a);
            for (int x = 0; x < width_; ++x) blendPixel(&row[x], v, a256);
        }
    }
}

void Canvas::fillRect(const Rect& r, const Color& c) {
    if (r.empty()) return;
    const Rect cr = clip().intersect(r);
    if (cr.empty()) return;

    const int x0 = clampi(static_cast<int>(std::floor(cr.left())), 0, width_);
    const int x1 = clampi(static_cast<int>(std::ceil(cr.right())), 0, width_);
    const int y0 = clampi(static_cast<int>(std::floor(cr.top())), 0, height_);
    const int y1 = clampi(static_cast<int>(std::ceil(cr.bottom())), 0, height_);
    if (x1 <= x0 || y1 <= y0) return;

    const uint32_t src = c.argb();
    const uint32_t a256 = coverageToA256(1.0f, c.a);

    if (a256 >= 256) {
        const uint32_t solid = 0xFF000000u | (src & 0x00FFFFFFu);
        for (int y = y0; y < y1; ++y) {
            uint32_t* row = pixels_ + static_cast<size_t>(y) * stride_;
            for (int x = x0; x < x1; ++x) row[x] = solid;
        }
    } else {
        for (int y = y0; y < y1; ++y) {
            uint32_t* row = pixels_ + static_cast<size_t>(y) * stride_;
            for (int x = x0; x < x1; ++x) blendPixel(&row[x], src, a256);
        }
    }
}

void Canvas::fillGradientV(const Rect& r, const Color& top, const Color& bottom) {
    if (r.empty()) return;
    const Rect cr = clip().intersect(r);
    if (cr.empty()) return;

    const int x0 = clampi(static_cast<int>(std::floor(cr.left())), 0, width_);
    const int x1 = clampi(static_cast<int>(std::ceil(cr.right())), 0, width_);
    const int y0 = clampi(static_cast<int>(std::floor(cr.top())), 0, height_);
    const int y1 = clampi(static_cast<int>(std::ceil(cr.bottom())), 0, height_);
    if (x1 <= x0 || y1 <= y0) return;

    for (int y = y0; y < y1; ++y) {
        const float t = clampf((static_cast<float>(y) + 0.5f - r.y) / r.h, 0.0f, 1.0f);
        const Color c = lerpColor(top, bottom, t);
        const uint32_t src = c.argb();
        const uint32_t a256 = coverageToA256(1.0f, c.a);
        uint32_t* row = pixels_ + static_cast<size_t>(y) * stride_;
        if (a256 >= 256) {
            const uint32_t solid = 0xFF000000u | (src & 0x00FFFFFFu);
            for (int x = x0; x < x1; ++x) row[x] = solid;
        } else {
            for (int x = x0; x < x1; ++x) blendPixel(&row[x], src, a256);
        }
    }
}

// --------------------------------------------------------------------------
template <typename ExtentFn>
void Canvas::fillShapeAA(const Rect& r, float innerInset, const Color& top,
                         const Color& bottom, bool gradient, ExtentFn extent) {
    if (r.empty() || r.w <= 0.0f || r.h <= 0.0f) return;

    const Rect cr = clip().intersect(r);
    if (cr.empty()) return;

    const float cx = r.x + r.w * 0.5f;
    const float cy = r.y + r.h * 0.5f;
    const float hw = r.w * 0.5f;
    const float hh = r.h * 0.5f;

    const int xLo = clampi(static_cast<int>(std::floor(cr.left())), 0, width_);
    const int xHi = clampi(static_cast<int>(std::ceil(cr.right())), 0, width_);
    const int yLo = clampi(static_cast<int>(std::floor(cr.top())), 0, height_);
    const int yHi = clampi(static_cast<int>(std::ceil(cr.bottom())), 0, height_);

    const bool hasInner = innerInset > 0.0f && (hw - innerInset) > 0.5f &&
                          (hh - innerInset) > 0.5f;
    const float ihw = hw - innerInset;
    const float ihh = hh - innerInset;

    constexpr int kSubs = 4;
    constexpr float kWeight = 1.0f / static_cast<float>(kSubs);

    for (int y = yLo; y < yHi; ++y) {
        float spans[kSubs][2];
        float innerSpans[kSubs][2];
        float minA = 1e30f, maxB = -1e30f;

        for (int s = 0; s < kSubs; ++s) {
            const float sy = static_cast<float>(y) + (static_cast<float>(s) + 0.5f) / kSubs;
            const float dy = sy - cy;

            const float e = clampf(extent(dy, hw, hh), 0.0f, 1.0f);
            spans[s][0] = cx - hw * e;
            spans[s][1] = cx + hw * e;
            minA = minf(minA, spans[s][0]);
            maxB = maxf(maxB, spans[s][1]);

            if (hasInner) {
                const float ei = clampf(extent(dy, ihw, ihh), 0.0f, 1.0f);
                innerSpans[s][0] = cx - ihw * ei;
                innerSpans[s][1] = cx + ihw * ei;
            }
        }

        const int ia = clampi(static_cast<int>(std::floor(minA)), xLo, xHi);
        const int ib = clampi(static_cast<int>(std::ceil(maxB)), xLo, xHi);
        if (ib <= ia) continue;

        for (int x = ia; x < ib; ++x) rowCov_[static_cast<size_t>(x)] = 0.0f;

        for (int s = 0; s < kSubs; ++s) {
            const float xa = spans[s][0];
            const float xb = spans[s][1];
            const int sa = clampi(static_cast<int>(std::floor(xa)), ia, ib);
            const int sb = clampi(static_cast<int>(std::ceil(xb)), ia, ib);
            for (int x = sa; x < sb; ++x) {
                const float l = maxf(xa, static_cast<float>(x));
                const float rr = minf(xb, static_cast<float>(x + 1));
                const float ov = rr - l;
                if (ov > 0.0f) rowCov_[static_cast<size_t>(x)] += ov * kWeight;
            }

            if (hasInner) {
                const float ixa = innerSpans[s][0];
                const float ixb = innerSpans[s][1];
                const int isa = clampi(static_cast<int>(std::floor(ixa)), ia, ib);
                const int isb = clampi(static_cast<int>(std::ceil(ixb)), ia, ib);
                for (int x = isa; x < isb; ++x) {
                    const float l = maxf(ixa, static_cast<float>(x));
                    const float rr = minf(ixb, static_cast<float>(x + 1));
                    const float ov = rr - l;
                    if (ov > 0.0f) rowCov_[static_cast<size_t>(x)] -= ov * kWeight;
                }
            }
        }

        uint32_t* row = pixels_ + static_cast<size_t>(y) * stride_;
        const Color col = gradient
                              ? lerpColor(top, bottom,
                                          clampf((static_cast<float>(y) + 0.5f - r.y) / r.h,
                                                 0.0f, 1.0f))
                              : top;
        const uint32_t src = col.argb();

        for (int x = ia; x < ib; ++x) {
            const float cov = rowCov_[static_cast<size_t>(x)];
            if (cov <= 0.0015f) continue;
            blendPixel(&row[x], src, coverageToA256(cov, col.a));
        }
    }
}

void Canvas::fillRoundRect(const Rect& r, float radius, const Color& c) {
    if (r.empty()) return;
    const float rad = clampf(radius, 0.0f, minf(r.w, r.h) * 0.5f);
    if (rad < 0.5f) {
        fillRect(r, c);
        return;
    }
    fillShapeAA(r, 0.0f, c, c, false,
                [rad](float dy, float hw, float hh) -> float {
                    const float dya = std::fabs(dy);
                    if (dya >= hh) return 0.0f;
                    if (dya <= hh - rad) return 1.0f;
                    const float dyc = dya - (hh - rad);
                    const float dx = rad - std::sqrt(maxf(0.0f, rad * rad - dyc * dyc));
                    return (hw - dx) / hw;
                });
}

void Canvas::fillRoundRectGradientV(const Rect& r, float radius, const Color& top,
                                    const Color& bottom) {
    if (r.empty()) return;
    const float rad = clampf(radius, 0.0f, minf(r.w, r.h) * 0.5f);
    fillShapeAA(r, 0.0f, top, bottom, true,
                [rad](float dy, float hw, float hh) -> float {
                    const float dya = std::fabs(dy);
                    if (dya >= hh) return 0.0f;
                    if (dya <= hh - rad) return 1.0f;
                    const float dyc = dya - (hh - rad);
                    const float dx = rad - std::sqrt(maxf(0.0f, rad * rad - dyc * dyc));
                    return (hw - dx) / hw;
                });
}

void Canvas::strokeRoundRect(const Rect& r, float radius, float thickness, const Color& c) {
    if (r.empty() || thickness <= 0.0f) return;
    const float rad = clampf(radius, 0.0f, minf(r.w, r.h) * 0.5f);
    const float hwOuter = r.w * 0.5f;
    // The inner outline is the same shape inset by `thickness`, which means its
    // corner radius shrinks by exactly that inset.
    fillShapeAA(r, thickness, c, c, false,
                [rad, hwOuter](float dy, float hw, float hh) -> float {
                    const float inR = maxf(0.0f, rad - (hwOuter - hw));
                    const float dya = std::fabs(dy);
                    if (dya >= hh) return 0.0f;
                    if (dya <= hh - inR) return 1.0f;
                    const float dyc = dya - (hh - inR);
                    const float dx = inR - std::sqrt(maxf(0.0f, inR * inR - dyc * dyc));
                    return (hw - dx) / hw;
                });
}

void Canvas::fillCircle(float cx, float cy, float radius, const Color& c) {
    fillRoundRect(Rect(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f), radius, c);
}

void Canvas::fillSquircle(const Rect& r, float n, const Color& c) {
    if (r.empty()) return;
    fillShapeAA(r, 0.0f, c, c, false,
                [n](float dy, float hw, float hh) -> float {
                    const float t = std::fabs(dy) / hh;
                    if (t >= 1.0f) return 0.0f;
                    return std::pow(maxf(0.0f, 1.0f - std::pow(t, n)), 1.0f / n);
                });
}

void Canvas::drawLine(float x0, float y0, float x1, float y1, float thickness,
                      const Color& c) {
    if (thickness <= 0.0f) return;

    const float half = thickness * 0.5f;
    const Rect bounds(minf(x0, x1) - half - 1.0f, minf(y0, y1) - half - 1.0f,
                      std::fabs(x1 - x0) + thickness + 2.0f,
                      std::fabs(y1 - y0) + thickness + 2.0f);

    const Rect cr = clip().intersect(bounds);
    if (cr.empty()) return;

    const float dx = x1 - x0;
    const float dy = y1 - y0;
    const float lenSq = dx * dx + dy * dy;
    const uint32_t src = c.argb();

    const int px0 = clampi(static_cast<int>(std::floor(cr.left())), 0, width_);
    const int px1 = clampi(static_cast<int>(std::ceil(cr.right())), 0, width_);
    const int py0 = clampi(static_cast<int>(std::floor(cr.top())), 0, height_);
    const int py1 = clampi(static_cast<int>(std::ceil(cr.bottom())), 0, height_);

    for (int y = py0; y < py1; ++y) {
        uint32_t* row = pixels_ + static_cast<size_t>(y) * stride_;
        for (int x = px0; x < px1; ++x) {
            const float px = static_cast<float>(x) + 0.5f - x0;
            const float py = static_cast<float>(y) + 0.5f - y0;

            float t = (lenSq > 0.0f) ? (px * dx + py * dy) / lenSq : 0.0f;
            t = clampf(t, 0.0f, 1.0f);

            const float qx = px - dx * t;
            const float qy = py - dy * t;
            const float dist = std::sqrt(qx * qx + qy * qy);

            const float coverage = clampf(half + 0.5f - dist, 0.0f, 1.0f);
            if (coverage <= 0.0f) continue;
            blendPixel(&row[x], src, coverageToA256(coverage, c.a));
        }
    }
}

void Canvas::strokeArc(float cx, float cy, float radius, float thickness, float startDeg,
                       float sweepDeg, const Color& c) {
    if (thickness <= 0.0f || radius <= 0.0f) return;

    const float outer = radius + thickness * 0.5f + 1.0f;
    const Rect bounds(cx - outer, cy - outer, outer * 2.0f, outer * 2.0f);
    const Rect cr = clip().intersect(bounds);
    if (cr.empty()) return;

    const float half = thickness * 0.5f;
    const uint32_t src = c.argb();

    // Normalise the sweep into a start/end pair in [0, 360).
    float a0 = std::fmod(startDeg, 360.0f);
    if (a0 < 0.0f) a0 += 360.0f;
    const float a1 = a0 + sweepDeg;
    const bool inverted = sweepDeg < 0.0f;

    const int px0 = clampi(static_cast<int>(std::floor(cr.left())), 0, width_);
    const int px1 = clampi(static_cast<int>(std::ceil(cr.right())), 0, width_);
    const int py0 = clampi(static_cast<int>(std::floor(cr.top())), 0, height_);
    const int py1 = clampi(static_cast<int>(std::ceil(cr.bottom())), 0, height_);

    for (int y = py0; y < py1; ++y) {
        uint32_t* row = pixels_ + static_cast<size_t>(y) * stride_;
        for (int x = px0; x < px1; ++x) {
            const float dx = static_cast<float>(x) + 0.5f - cx;
            const float dy = static_cast<float>(y) + 0.5f - cy;

            const float dist = std::sqrt(dx * dx + dy * dy);
            const float ring = std::fabs(dist - radius);
            const float coverage = clampf(half + 0.5f - ring, 0.0f, 1.0f);
            if (coverage <= 0.0f) continue;

            float angle = std::atan2(dy, dx) * 180.0f / kPi;
            if (angle < 0.0f) angle += 360.0f;

            bool inside;
            if (inverted) {
                // Sweeping backwards from a0 down to a1.
                float rel = a0 - angle;
                if (rel < 0.0f) rel += 360.0f;
                inside = rel <= -sweepDeg;
            } else {
                float rel = angle - a0;
                if (rel < 0.0f) rel += 360.0f;
                inside = rel <= sweepDeg;
            }
            if (!inside) continue;

            blendPixel(&row[x], src, coverageToA256(coverage, c.a));
        }
    }
}

// --------------------------------------------------------------------------
const Canvas::ShadowMask* Canvas::shadowMask(float w, float h, float radius, float n,
                                            float blur, bool squircle) {
    const uint64_t key = hashShape(w, h, radius, n, blur, squircle);
    for (const ShadowMask& m : shadowCache_) {
        if (m.key == key) return &m;
    }

    const int pad = static_cast<int>(std::ceil(blur * 1.5f)) + 2;
    const int mw = static_cast<int>(std::ceil(w)) + pad * 2;
    const int mh = static_cast<int>(std::ceil(h)) + pad * 2;
    if (mw <= 0 || mh <= 0 || mw > 4096 || mh > 4096) return nullptr;

    ShadowMask mask;
    mask.key = key;
    mask.w = mw;
    mask.h = mh;
    mask.pad = pad;
    mask.alpha.assign(static_cast<size_t>(mw) * mh, 0);

    const float hw = w * 0.5f;
    const float hh = h * 0.5f;

    for (int y = 0; y < mh; ++y) {
        const float cy = static_cast<float>(y) - static_cast<float>(pad) - hh + 0.5f;
        for (int x = 0; x < mw; ++x) {
            const float cx = static_cast<float>(x) - static_cast<float>(pad) - hw + 0.5f;
            const float d = squircle ? sdfSquircle(cx, cy, hw, hh, n)
                                     : sdfRoundRect(cx, cy, hw, hh, radius);
            float a;
            if (d <= 0.0f) {
                a = 1.0f;
            } else if (d >= blur) {
                a = 0.0f;
            } else {
                const float t = d / blur;
                a = 1.0f - (t * t * (3.0f - 2.0f * t));
            }
            mask.alpha[static_cast<size_t>(y) * mw + x] =
                static_cast<uint8_t>(clampf(a, 0.0f, 1.0f) * 255.0f + 0.5f);
        }
    }

    shadowCache_.push_back(std::move(mask));
    return &shadowCache_.back();
}

void Canvas::drawShadow(const ShadowMask* mask, float x, float y, const Color& c) {
    if (!mask) return;
    blendMask(mask->alpha.data(), mask->w, mask->w, mask->h, x - mask->pad,
              y - mask->pad, c);
}

void Canvas::shadowRoundRect(const Rect& r, float radius, float blur, const Color& c,
                            float dx, float dy) {
    const ShadowMask* mask =
        shadowMask(r.w, r.h, clampf(radius, 0.0f, minf(r.w, r.h) * 0.5f), 4.0f, blur, false);
    drawShadow(mask, r.x + dx, r.y + dy, c);
}

void Canvas::shadowSquircle(const Rect& r, float n, float blur, const Color& c, float dx,
                           float dy) {
    const ShadowMask* mask = shadowMask(r.w, r.h, 0.0f, n, blur, true);
    drawShadow(mask, r.x + dx, r.y + dy, c);
}

// --------------------------------------------------------------------------
void Canvas::blendMask(const uint8_t* mask, int maskStride, int mw, int mh, float dstX,
                       float dstY, const Color& color) {
    if (!mask || mw <= 0 || mh <= 0) return;

    const Rect cr = clip();
    const int x0 = clampi(static_cast<int>(std::floor(dstX)), 0, width_);
    const int x1 = clampi(static_cast<int>(std::ceil(dstX + mw)), 0, width_);
    const int y0 = clampi(static_cast<int>(std::floor(dstY)), 0, height_);
    const int y1 = clampi(static_cast<int>(std::ceil(dstY + mh)), 0, height_);
    if (x1 <= x0 || y1 <= y0) return;

    const int ox = static_cast<int>(std::lround(dstX));
    const int oy = static_cast<int>(std::lround(dstY));
    const uint32_t src = color.argb();

    for (int y = y0; y < y1; ++y) {
        const int my = y - oy;
        if (my < 0 || my >= mh) continue;
        const uint8_t* mrow = mask + static_cast<size_t>(my) * maskStride;
        uint32_t* row = pixels_ + static_cast<size_t>(y) * stride_;
        for (int x = x0; x < x1; ++x) {
            if (x < static_cast<int>(cr.left()) || x >= static_cast<int>(cr.right())) continue;
            const int mx = x - ox;
            if (mx < 0 || mx >= mw) continue;
            const uint8_t m = mrow[mx];
            if (m == 0) continue;
            const uint32_t a256 =
                (static_cast<uint32_t>(m) * color.a * 256u) / (255u * 255u);
            blendPixel(&row[x], src, a256);
        }
    }
}

}  // namespace rawui
